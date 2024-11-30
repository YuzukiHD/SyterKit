import re

# Define the file path
file_path = 'sys-dram.asm.c'  # Replace with your filename

# Read the file content
with open(file_path, 'r') as file:
    content = file.readlines()

# Replace the content
updated_content = []
for line in content:
    # Use regex to find _DAT_ and replace it correctly
    updated_line = re.sub(r'(_DAT_|uRam)(0x?\w+)', r'REG32(0x\2)', line)
    updated_content.append(updated_line)

# Write the modified content back to the file
with open(file_path, 'w') as file:
    file.writelines(updated_content)

print("Replacement complete!")