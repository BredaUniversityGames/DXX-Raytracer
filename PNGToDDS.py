import os
import sys

dirName = sys.argv[1] # Get the name of the folder passed to this script
print(f'Creating batch script for directory {dirName}')

filesInDir = os.listdir(dirName) # Get all files in the directory
filesInDir = [f for f in filesInDir if f.lower().endswith('png')] # Get only PNG files
filesInDir = [os.path.join(dirName, f) for f in filesInDir] # Get absolute paths
filesInDir = [f for f in filesInDir if os.path.isfile(f)] # Ignore directories

outScript = open('DXX_Raytracer_Convert_PNG_To_DDS.nvtt', 'w') # Start a new script

baseColorTexturesInDir = [f for f in filesInDir if f.find("basecolor")]

for file in baseColorTexturesInDir:
  newFileName = file.rpartition('.')[0] + '.dds' # Replace .png with .dds
  outScript.write(f'{file} --format bc7 --dx10 --mips --output {newFileName}\n')

metallicTexturesInDir = [f for f in filesInDir if f.find("metallic")]

for file in metallicTexturesInDir:
  newFileName = file.rpartition('.')[0] + '.dds' # Replace .png with .dds
  outScript.write(f'{file} --format bc4 --dx10 --mips --output {newFileName}\n')

roughnessTexturesInDir = [f for f in filesInDir if f.find("roughness")]

for file in roughnessTexturesInDir:
  newFileName = file.rpartition('.')[0] + '.dds' # Replace .png with .dds
  outScript.write(f'{file} --format bc4 --dx10 --mips --output {newFileName}\n')

normalTexturesInDir = [f for f in filesInDir if f.find("normal")]

for file in normalTexturesInDir:
  newFileName = file.rpartition('.')[0] + '.dds' # Replace .png with .dds
  outScript.write(f'{file} --format bc7 --dx10 --mips --output {newFileName}\n')

emissiveTexturesInDir = [f for f in filesInDir if f.find("emissive")]

for file in emissiveTexturesInDir:
  newFileName = file.rpartition('.')[0] + '.dds' # Replace .png with .dds
  outScript.write(f'{file} --format bc7 --export-pre-alpha --dx10 --mips --output {newFileName}\n')

outScript.close()
print('Done')