#include <sstream>
#include "MappingContainer.h"
#include "vlq.h"

MappingContainer::MappingContainer() {}

MappingContainer::~MappingContainer() {}

void MappingContainer::Finalize() {}

void MappingContainer::reserve(size_t size) {
    _mappings.reserve(size);
}

int MappingContainer::getNames() {
    return _names;
}

void MappingContainer::addNames(int amount) {
    _names += amount;
}

int MappingContainer::getSources() {
    return _sources;
}

void MappingContainer::addSources(int amount) {
    _sources += amount;
}

int MappingContainer::getGeneratedLines() {
    return _generated_lines;
}

int MappingContainer::getGeneratedColumns() {
    return _generated_columns;
}

void MappingContainer::addMapping(Position generated, Position original, int source, int name) {
    if (generated.line > _generated_lines) {
        _generated_lines = generated.line;
    }

    if (generated.column > _generated_columns) {
        _generated_columns = generated.column;
    }

    Mapping m = {
            .generated = generated,
            .original = original,
            .source = source,
            .name = name
    };

    _mappings.push_back(m);
}

void MappingContainer::addMappingBySegment(int generatedLine, int *segment, int segmentIndex) {
    bool hasSource = segmentIndex > 3;
    bool hasName = segmentIndex > 4;

    Position generated = {
            .line = generatedLine,
            .column = segment[0]
    };

    Position original = {
            .line = hasSource ? segment[2] : -1,
            .column = hasSource ? segment[3] : -1
    };

    this->addMapping(generated, original, hasSource ? segment[1] : -1, hasName ? segment[4] : -1);
}

void MappingContainer::addVLQMappings(const std::string &mappings_input, int line_offset, int column_offset) {
    // SourceMap information
    int generatedLine = line_offset;

    // VLQ Decoding
    int value = 0;
    int shift = 0;
    int segment[5] = {column_offset, _sources, 0, 0, _names};
    int segmentIndex = 0;

    // `input.len() / 2` is the upper bound on how many mappings the string
    // might contain. There would be some sequence like `A,A,...` or `A;A...`
    _mappings.reserve(mappings_input.length() / 2);

    auto it = mappings_input.begin();
    auto end = mappings_input.end();
    for (; it != end; ++it) {
        const char c = *it;
        if (c == ',' || c == ';') {
            this->addMappingBySegment(generatedLine, segment, segmentIndex);

            if (c == ';') {
                segment[0] = 0;
                generatedLine++;
            }

            segmentIndex = 0;
            continue;
        }

        const int decodedCharacter = decodeBase64Char(c);

        value += (decodedCharacter & VLQ_BASE_MASK) << shift;

        if ((decodedCharacter & VLQ_BASE) != 0) {
            shift += VLQ_BASE_SHIFT;
        } else {
            // The low bit holds the sign.
            if ((value & 1) != 0) {
                value = -value;
            }

            segment[segmentIndex++] += value / 2;
            shift = value = 0;
        }
    }

    // Process last mapping...
    if (segmentIndex > 0) {
        this->addMappingBySegment(generatedLine, segment, segmentIndex);
    }
}

std::string MappingContainer::toVLQMappings() {
    std::stringstream out;
    int previousGeneratedLine = 0;
    int previousGeneratedColumn = 0;
    int previousSource = 0;
    int previousOriginalLine = 0;
    int previousOriginalColumn = 0;
    int previousName = 0;
    bool isFirst = true;

    auto it = _mappings.begin();
    auto end = _mappings.end();
    for (; it != end; ++it) {
        Mapping mapping = *it;

        if (previousGeneratedLine < mapping.generated.line) {
            previousGeneratedColumn = 0;
            isFirst = true;

            while (previousGeneratedLine < mapping.generated.line) {
                out << ";";
                previousGeneratedLine++;
            }
        }

        if (!isFirst) {
            out << ",";
        }

        encodeVlq(mapping.generated.column - previousGeneratedColumn, out);
        previousGeneratedColumn = mapping.generated.column;

        if (mapping.source > -1) {
            encodeVlq(mapping.source - previousSource, out);
            previousSource = mapping.source;

            encodeVlq(mapping.original.line - previousOriginalLine, out);
            previousOriginalLine = mapping.original.line;

            encodeVlq(mapping.original.column - previousOriginalColumn, out);
            previousOriginalColumn = mapping.original.column;
        }

        if (mapping.name > -1) {
            encodeVlq(mapping.name - previousName, out);
            previousName = mapping.name;
        }

        isFirst = false;
    }

    return out.str();
}

std::string MappingContainer::debugString() {
    std::stringstream out;

    auto it = this->_mappings.begin();
    auto end = this->_mappings.end();
    for (; it != end; ++it) {
        Mapping mapping = *it;
        out << "==== Start Mapping ====" << std::endl;
        out << "Generated line: " << mapping.generated.line << std::endl;
        out << "Generated column: " << mapping.generated.column << std::endl;
        out << "Source: " << mapping.source << std::endl;
        out << "Original line: " << mapping.original.line << std::endl;
        out << "Original column: " << mapping.original.column << std::endl;
        out << "Name: " << mapping.name << std::endl;
    }
    return out.str();
}

std::vector<Mapping> &MappingContainer::getMappingsVector() {
    return this->_mappings;
}