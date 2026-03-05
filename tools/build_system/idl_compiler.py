"""
IDL Compiler Module
Generates C++ code from IDL topic definitions
Integrated into the Devana build system
"""

import re
from dataclasses import dataclass, field as dataclass_field
from typing import List, Dict, Optional, Tuple
from pathlib import Path


@dataclass
class Field:
    name: str
    type: str
    is_required: bool = False
    is_optional: bool = False
    default_value: Optional[str] = None
    range_min: Optional[int] = None
    range_max: Optional[int] = None
    description: Optional[str] = None


@dataclass
class Message:
    name: str
    fields: List[Field]
    priority: Optional[str] = None
    version: Optional[str] = None
    description: Optional[str] = None


@dataclass
class Enum:
    name: str
    values: Dict[str, int]


@dataclass
class Topic:
    name: str
    namespace: str
    messages: List[Message] = dataclass_field(default_factory=list)
    enums: List[Enum] = dataclass_field(default_factory=list)
    imports: List[str] = dataclass_field(default_factory=list)
    description: Optional[str] = None
    qos: Optional[Dict] = None


class IDLParser:
    TYPE_MAP = {
        'string': 'std::string',
        'bool': 'bool',
        'uint8': 'uint8_t',
        'uint16': 'uint16_t',
        'uint32': 'uint32_t',
        'uint64': 'uint64_t',
        'int8': 'int8_t',
        'int16': 'int16_t',
        'int32': 'int32_t',
        'int64': 'int64_t',
        'float': 'float',
        'double': 'double',
    }

    def parse(self, idl_file: Path) -> Topic:
        with open(idl_file) as f:
            content = f.read()

        content = re.sub(r'//.*', '', content)

        namespace_match = re.search(r'namespace\s+([\w.]+);', content)
        namespace = namespace_match.group(1) if namespace_match else ""

        imports = re.findall(r'import\s+"([^"]+)";', content)

        topic_pattern = r'(?:@description\("([^"]+)"\)\s*)?(?:@qos\(([^)]+)\)\s*)?topic\s+(\w+)\s*{'
        topic_match = re.search(topic_pattern, content)

        if not topic_match:
            raise ValueError(f"No topic definition found in {idl_file}")

        topic_name = topic_match.group(3)
        topic_desc = topic_match.group(1)

        messages = self._parse_messages(content)
        enums = self._parse_enums(content)

        return Topic(
            name=topic_name,
            namespace=namespace,
            messages=messages,
            enums=enums,
            imports=imports,
            description=topic_desc
        )

    def _parse_messages(self, content: str) -> List[Message]:
        messages = []
        pattern = r'((?:@\w+\([^)]*\)\s*)*)\s*message\s+(\w+)\s*{([^}]+)}'

        for match in re.finditer(pattern, content):
            annotations = match.group(1)
            msg_name = match.group(2)
            msg_body = match.group(3)

            priority = self._extract_annotation(annotations, 'priority')
            version = self._extract_annotation(annotations, 'version')
            description = self._extract_annotation(annotations, 'description')
            fields = self._parse_fields(msg_body)

            messages.append(Message(
                name=msg_name,
                fields=fields,
                priority=priority,
                version=version,
                description=description
            ))

        return messages

    def _parse_fields(self, msg_body: str) -> List[Field]:
        fields = []
        lines = msg_body.strip().split(';')

        for line in lines:
            line = line.strip()
            if not line:
                continue

            is_required = '@required' in line
            is_optional = '@optional' in line

            default_match = re.search(r'@default\(([^)]+)\)', line)
            default_value = default_match.group(1) if default_match else None

            range_match = re.search(r'@range\((\d+),\s*(\d+)\)', line)
            range_min = int(range_match.group(1)) if range_match else None
            range_max = int(range_match.group(2)) if range_match else None

            field_match = re.search(r'(\w+)\s+(\w+)\s*$', line)
            if field_match:
                field_type = field_match.group(1)
                field_name = field_match.group(2)
                cpp_type = self.TYPE_MAP.get(field_type, field_type)

                fields.append(Field(
                    name=field_name,
                    type=cpp_type,
                    is_required=is_required,
                    is_optional=is_optional,
                    default_value=default_value,
                    range_min=range_min,
                    range_max=range_max
                ))

        return fields

    def _parse_enums(self, content: str) -> List[Enum]:
        enums = []
        pattern = r'enum\s+(\w+)\s*{([^}]+)}'
        for match in re.finditer(pattern, content):
            enum_name = match.group(1)
            enum_body = match.group(2)

            values = {}
            for line in enum_body.strip().split(','):
                line = line.strip()
                if not line:
                    continue
                value_match = re.search(r'(\w+)\s*=\s*(\d+)', line)
                if value_match:
                    values[value_match.group(1)] = int(value_match.group(2))

            enums.append(Enum(name=enum_name, values=values))

        return enums

    def _extract_annotation(self, text: str, name: str) -> Optional[str]:
        match = re.search(rf'@{name}\(([^)]+)\)', text)
        if match:
            value = match.group(1).strip()
            if value.startswith('"') and value.endswith('"'):
                value = value[1:-1]
            return value
        return None


class CppGenerator:
    def generate_header(self, topic: Topic, output_path: Path):
        cpp_namespace = topic.namespace.replace('.', '::') if topic.namespace else 'devana'
        guard = f"{topic.name.upper()}_H"

        code = f"""/**
 * Auto-generated from {topic.name}.idl
 * DO NOT EDIT - Changes will be overwritten by IDL compiler
 */

#ifndef {guard}
#define {guard}

#include "core/Transport/SocketTransport.h"
#include <string>
#include <cstdint>
#include <optional>
#include <vector>
#include <stdexcept>

"""
        for imp in topic.imports:
            header_path = imp.replace('.idl', '.h')
            code += f'#include "{header_path}"\n'

        if topic.imports:
            code += '\n'

        code += f"namespace {cpp_namespace}::topics::{topic.name} {{\n\n"

        for enum in topic.enums:
            code += self._generate_enum(enum)

        for msg in topic.messages:
            code += self._generate_message(msg)

        code += self._generate_writer(topic)
        code += self._generate_reader(topic)
        code += self._generate_utils(topic)

        code += f"}} // namespace {cpp_namespace}::topics::{topic.name}\n\n"
        code += f"#endif // {guard}\n"

        output_path.write_text(code)

    def _generate_enum(self, enum: Enum) -> str:
        code = f"enum class {enum.name} : uint8_t {{\n"
        for name, value in enum.values.items():
            code += f"    {name} = {value},\n"
        code += "};\n\n"
        return code

    def _generate_message(self, msg: Message) -> str:
        code = f"/**\n * Message: {msg.name}\n"
        if msg.description:
            code += f" * {msg.description}\n"
        code += " */\n"
        code += f"struct {msg.name} {{\n"

        for field in msg.fields:
            if field.is_optional:
                code += f"    std::optional<{field.type}> {field.name};\n"
            else:
                code += f"    {field.type} {field.name}"
                if field.default_value is not None:
                    code += f" = {field.default_value}"
                code += ";\n"

        code += "\n    bool isValid() const {\n"
        has_validation = False

        for field in msg.fields:
            if field.is_required:
                has_validation = True
                if field.type == "std::string":
                    code += f"        if ({field.name}.empty()) return false;\n"
            if field.range_min is not None and field.range_max is not None:
                has_validation = True
                code += f"        if ({field.name} < {field.range_min} || {field.name} > {field.range_max}) return false;\n"

        if not has_validation:
            code += "        // No validation rules defined\n"

        code += "        return true;\n    }\n\n"
        code += self._generate_builder(msg)
        code += "};\n\n"
        return code

    def _generate_builder(self, msg: Message) -> str:
        code = "    class Builder {\n    public:\n"

        for field in msg.fields:
            setter_name = field.name[0].upper() + field.name[1:]
            param_type = f"std::optional<{field.type}>" if field.is_optional else field.type

            code += f"        Builder& set{setter_name}({param_type} val) {{\n"

            if field.range_min is not None and field.range_max is not None and not field.is_optional:
                code += f"            if (val < {field.range_min} || val > {field.range_max}) {{\n"
                code += f'                throw std::invalid_argument("{field.name} must be between {field.range_min} and {field.range_max}");\n'
                code += "            }\n"

            code += f"            msg_.{field.name} = std::move(val);\n"
            code += "            return *this;\n        }\n"

        code += f"\n        {msg.name} build() {{\n"
        code += "            if (!msg_.isValid()) {\n"
        code += '                throw std::runtime_error("Message validation failed");\n'
        code += "            }\n            return std::move(msg_);\n        }\n\n"
        code += "    private:\n"
        code += f"        {msg.name} msg_;\n    }};\n\n"
        return code

    def _generate_writer(self, topic: Topic) -> str:
        code = f"class Writer {{\npublic:\n"
        code += f'    explicit Writer(void* owner, const std::string& topicName = "{topic.name}")\n'
        code += "        : owner_(owner), topicName_(topicName) {}\n\n"

        for msg in topic.messages:
            writer_name = msg.name[0].lower() + msg.name[1:]
            code += f"    auto& {writer_name}() {{\n"
            code += f"        if (!{writer_name}Writer_) {{\n"
            code += f"            {writer_name}Writer_ = TheSocketTransport->getWriter<{msg.name}>(owner_, \"{msg.name}\", topicName_);\n"
            code += "        }\n"
            code += f"        return *{writer_name}Writer_;\n    }}\n\n"
            code += f"    void publish(const {msg.name}& msg) {{ {writer_name}().write(msg); }}\n\n"

        code += "private:\n    void* owner_;\n    std::string topicName_;\n\n"
        for msg in topic.messages:
            writer_name = msg.name[0].lower() + msg.name[1:]
            code += f"    std::shared_ptr<MessageWriter<{msg.name}>> {writer_name}Writer_;\n"
        code += "};\n\n"
        return code

    def _generate_reader(self, topic: Topic) -> str:
        code = f"class Reader {{\npublic:\n"
        code += f'    explicit Reader(void* owner, const std::string& topicName = "{topic.name}")\n'
        code += "        : owner_(owner), topicName_(topicName) {}\n\n"

        for msg in topic.messages:
            code += f"    template<typename T>\n"
            code += f"    Reader& on{msg.name}(T* obj, void (T::*func)(const {msg.name}&)) {{\n"
            code += f'        TheSocketTransport->addSink<{msg.name}>(obj, func, "{msg.name}");\n'
            code += "        return *this;\n    }\n\n"
            code += f"    Reader& on{msg.name}(std::function<void(const {msg.name}&)> callback) {{\n"
            code += f'        TheSocketTransport->addSink<{msg.name}>(owner_, callback, "{msg.name}");\n'
            code += "        return *this;\n    }\n\n"

        code += "private:\n    void* owner_;\n    std::string topicName_;\n};\n\n"
        return code

    def _generate_utils(self, topic: Topic) -> str:
        code = "namespace utils {\n\n"
        code += "inline std::vector<std::string> getMessageTypes() {\n    return {\n"
        for msg in topic.messages:
            code += f'        "{msg.name}",\n'
        code += "    };\n}\n\n"

        if topic.description:
            code += f"inline const char* getDescription() {{ return \"{topic.description}\"; }}\n\n"

        code += f"inline const char* getNamespace() {{ return \"{topic.namespace}\"; }}\n\n"
        code += f"inline const char* getTopicName() {{ return \"{topic.name}\"; }}\n\n"
        code += "} // namespace utils\n\n"
        return code


def compile_idl(idl_file: Path, output_dir: Path = None, verbose: bool = False) -> Tuple[bool, str]:
    try:
        if not idl_file.exists():
            return False, f"IDL file not found: {idl_file}"

        if verbose:
            print(f"Parsing {idl_file}...")

        parser = IDLParser()
        topic = parser.parse(idl_file)

        if verbose:
            print(f"  Topic: {topic.name}")
            print(f"  Namespace: {topic.namespace}")
            print(f"  Messages: {len(topic.messages)}")
            print(f"  Enums: {len(topic.enums)}")

        if output_dir:
            output_path = output_dir / f"{topic.name}.h"
        else:
            output_path = Path("src/topics") / f"{topic.name}.h"

        output_path.parent.mkdir(parents=True, exist_ok=True)

        if verbose:
            print(f"Generating {output_path}...")

        generator = CppGenerator()
        generator.generate_header(topic, output_path)

        return True, str(output_path)

    except Exception as e:
        return False, f"Error: {e}"


def compile_all_idl(topics_dir: Path, output_dir: Path = None, verbose: bool = False) -> Tuple[int, int, List[str]]:
    if not topics_dir.exists():
        return 0, 0, [f"Topics directory not found: {topics_dir}"]

    idl_files = list(topics_dir.glob("*.idl"))

    if not idl_files:
        return 0, 0, [f"No IDL files found in {topics_dir}"]

    success_count = 0
    fail_count = 0
    errors = []

    for idl_file in idl_files:
        success, message = compile_idl(idl_file, output_dir, verbose)
        if success:
            success_count += 1
        else:
            fail_count += 1
            errors.append(f"{idl_file.name}: {message}")

    return success_count, fail_count, errors
