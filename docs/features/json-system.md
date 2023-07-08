# JSON System

The Half-Life Unified SDK uses JSON for configuration files.

[NLohmann JSON](https://github.com/nlohmann/json) is used to load JSON files and [PBoettch json-schema-validator](https://github.com/pboettch/json-schema-validator) to validate them.

While not officially supported, various JSON parsers support comments so single line comments are used in these files.

All JSON configuration files must use UTF8 encoding.

The JSON system uses the logger named `json`.

## Parse error reporting

If a JSON file has malformed syntax the error will be reported using an error code.

For example a forgotten comma results in this error being shown:
```
[sv] [json] [error] Error "101" parsing JSON: "[json.exception.parse_error.101] parse error at line 11, column 3: syntax error while parsing array - unexpected '{'; expected ']'"
```

See the [NLohmann JSON documentation](https://json.nlohmann.me/home/exceptions/#jsonexceptionparse_error101) for more information about which error types exist and what they mean.

## JSON Schema validation

To aid in finding problems in configuration files all files have a [JSON Schema](https://json-schema.org) that specifies the format for that file.

[JSON Schema draft 7](http://json-schema.org/draft-07/schema) is used to perform validation.

Setting `json_schema_validation` to `1` enables validation.

The validation system reports errors by showing the path in the file and the JSON value (truncated if it exceeds 100 bytes).

For example an invalid `Name` value results in this error being shown:
```
[sv] [json] [error] Error validating JSON "/Sections/1" with value "{"Color":"255 160 0","Name":"foo"}": no subschema has succeeded, but one of them is required to validate
```

This indicates that the second array element (starting at 0 for the first) of the `Sections` array is invalid.

See the console commands below for more information on where to get the schemas used to perform this validation.

## Console commands

Since the JSON system is available on both client and server all commands are prefixed with either `cl_` or `sv_`.

### json_listschemas

Syntax: `json_listschemas`

Prints a list of all JSON schemas.

Only available if `json_debug` is enabled.

### json_generateschema

Syntax: `json_generateschema <schema_name>`

Generates a file in `schemas/<shortlibraryprefix>` containing the given JSON schema.

Only available if `json_debug` is enabled.

### json_generateallschemas

Syntax: `json_generateallschemas`

Generates files for all schemas in `schemas/<shortlibraryprefix>`.

Only available if `json_debug` is enabled.

## Console variables

### json_debug

Boolean value that controls whether JSON debug console commands are enabled.

### json_schema_validation

Boolean value that controls whether JSON Schema validation is enabled.
