import argparse
import json


# Parse the configuration file and extract variables starting with "CONFIG_"
def parse_config_file(config_file_path):
    config = {}  # Create an empty dictionary to store configuration items
    with open(config_file_path, "r") as file:
        for line in file:  # Read the file line by line
            line = line.strip()  # Remove any leading/trailing whitespace
            if line.startswith("CONFIG_"):  # Process only lines that start with CONFIG_
                # Split the line into key and value at the first "="
                key, value = line.split("=", 1)
                config[key] = value  # Store the configuration item in the dictionary
    return config


# Generate the content for c_cpp_properties.json based on the parsed configuration
def generate_c_cpp_properties(config):
    defines = []  # List to store macro definitions
    include_paths = ['${workspaceFolder}/**']

    for key, _ in config.items():
        defines.append(key.strip())
    # Structure the content for c_cpp_properties.json
    cpp_properties = {
        "configurations": [
            {
                "name": "SyterKit",
                "includePath": include_paths,
                "defines": defines
            }
        ],
        "version": 4
    }
    return cpp_properties


# Write the generated c_cpp_properties.json content to a file
def write_c_cpp_properties_json(output_file_path, cpp_properties):
    try:
        with open(output_file_path, "w") as json_file:
            json.dump(cpp_properties, json_file, indent=4)
    except FileNotFoundError:
        pass


# Main function to execute the process
def main():
    # Set up argument parsing
    parser = argparse.ArgumentParser(
        description="Generate c_cpp_properties.json from a configuration file."
    )
    parser.add_argument("config_file", help="Path to the configuration file.")
    parser.add_argument(
        "output_file", help="Path for the output c_cpp_properties.json file."
    )

    args = parser.parse_args()  # Parse the arguments

    # Parse the configuration file
    config = parse_config_file(args.config_file)

    # Generate the c_cpp_properties.json content
    cpp_properties = generate_c_cpp_properties(config)

    # Write the content to a JSON file
    write_c_cpp_properties_json(args.output_file, cpp_properties)


if __name__ == "__main__":
    main()  # Execute the main function
