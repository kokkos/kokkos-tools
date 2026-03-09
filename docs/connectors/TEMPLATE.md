# Connector Documentation Template

This template provides a structure for documenting third-party library (TPL) connectors for Kokkos Tools. Use this as a guide when creating documentation for your connector.

---

# [Connector Name] Connector

## Overview

[Provide a brief description of the tool/library. Include:]
- What is the tool? (1-2 sentences)
- What are its main capabilities?
- How does the Kokkos connector integrate it?

Example:
> [ToolName](https://example.com) is a [description]. The [ToolName] Kokkos Tools connector brings [ToolName]'s capabilities to Kokkos applications by invoking [ToolName]'s functions through callbacks corresponding to Kokkos library functions.

## Key Features

[List the main features and capabilities of the connector. Use bullet points for clarity.]

- **Feature 1**: Description
- **Feature 2**: Description
- **Feature 3**: Description

## Use Cases

[Describe when and why users should consider this connector:]
- Use case 1
- Use case 2
- Use case 3

[Optional: Note any limitations or specific focus areas]

## Installation and Setup

### Prerequisites

[List prerequisites needed:]
- Specific version requirements
- System requirements
- Kokkos configuration requirements

### Building the Connector

[Provide build instructions. Include:]
- Where to get the source code
- Build commands
- Configuration options

Example:
```bash
# Clone or download
git clone https://example.com/connector.git

# Build
mkdir build && cd build
cmake ..
make
```

### Using the Connector

[Provide usage instructions:]

1. Set environment variables
2. Run your application
3. Access output

Example:
```bash
export KOKKOS_TOOLS_LIBS=/path/to/connector.so
# Additional environment variables
./your_kokkos_application
```

## Configuration

[If applicable, document configuration options:]

### Environment Variables

| Variable | Description | Default | Example |
|----------|-------------|---------|---------|
| VAR_NAME | What it controls | default_value | example_value |

### Configuration Files

[If the connector uses configuration files, describe them here]

## Output

[Describe what output users should expect:]
- Output location
- Output format(s)
- How to interpret results
- Any post-processing tools available

## Advanced Usage

[Optional: Include advanced features or usage patterns]

### Custom Analysis

[If applicable, describe how users can customize analysis]

### Integration with Other Tools

[If the connector integrates with other tools, describe how]

## Resources and Documentation

### Official Resources

- **Website**: [link]
- **Documentation**: [link to official docs]
- **Repository**: [link to source code]
- **Support**: [link to support channels]

### Kokkos-Specific Resources

- **Tutorial**: [link if available]
- **Examples**: [link to example code if available]
- **Wiki Page**: [link to Kokkos Tools wiki page]

## Troubleshooting

[Optional but recommended: Common issues and solutions]

### Common Issues

**Problem**: [Description]
**Solution**: [How to fix it]

## Contributing

[Information about how users can contribute to the connector or its documentation]

## Support and Contact

[How users can get help:]
- For connector-specific issues: [contact/link]
- For Kokkos Tools questions: See main [Kokkos Tools README](../../README.md)

## See Also

- [Other Connector Name](OtherConnector.md) - Brief description
- [Kokkos Tools Wiki](https://github.com/kokkos/kokkos-tools/wiki)
- [Main Kokkos Tools Documentation](../README.md)

---

## Notes for Documentation Contributors

When using this template:

1. **Fill in all sections** - Replace bracketed placeholders with actual content
2. **Keep it concise** - Aim for clarity and completeness without excessive detail
3. **Link to external docs** - Reference comprehensive external documentation rather than duplicating it
4. **Use examples** - Include practical code snippets and examples where helpful
5. **Test the instructions** - Verify that build and usage instructions work
6. **Keep it updated** - Update documentation when the connector changes

### Formatting Tips

- Use headers (`##`, `###`) to organize content hierarchically
- Use **bold** for emphasis on key terms
- Use `code blocks` for commands, file paths, and code
- Use bullet points for lists
- Use tables for structured data (environment variables, options)
- Include links to relevant resources

### Optional Sections

Not all sections may be relevant for every connector. Optional sections include:
- Troubleshooting
- Advanced Usage
- Configuration Files
- Custom Analysis

Feel free to add connector-specific sections as needed.
