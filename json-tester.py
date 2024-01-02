import json

csharp_json = R"C:\Users\PatrikHuber\Projects\Facebow\tmp-out\output1.json"
cpp_json = R"C:\Users\PatrikHuber\Projects\Facebow\FacebowFileReader\out\build\x64-Debug\key.json"

csharp_file = open(csharp_json)
csharp_data = json.load(csharp_file)

cpp_file = open(cpp_json)
cpp_data = json.load(cpp_file)

csharp_pretty = json.dumps(csharp_data, indent=2, sort_keys=True)
cpp_pretty = json.dumps(cpp_data, indent=2, sort_keys=True)

# csharp_pretty = csharp_pretty.replace('\\n', '\n')
# cpp_pretty = cpp_pretty.replace('\\n', '\n')

csharp_file_out = open('csharp_pretty.json', 'wt', encoding='utf-8')
# json.dump(csharp_pretty, csharp_file_out, indent=2)
csharp_file_out.write(csharp_pretty)

cpp_file_out = open('cpp_pretty.json', 'wt', encoding='utf-8')
# json.dump(cpp_pretty, cpp_file_out, indent=2)
cpp_file_out.write(cpp_pretty)
