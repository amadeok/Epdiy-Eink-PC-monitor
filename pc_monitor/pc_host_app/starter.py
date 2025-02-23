import argparse
import subprocess
import tempfile

import configparser
import time
import get_board_ips
import ast
import kill_all_proc_captures

kill_all_proc_captures.kill_all()

def parse_list(s):
    try:  return ast.literal_eval(s)
    except (SyntaxError, ValueError) as e: raise argparse.ArgumentTypeError(f"Invalid list: {s}")
    
parser = argparse.ArgumentParser(
                    prog='eink display starter',
                    description='starts eink pc host apps from human friendly arguments',
                    epilog='')
                    
# parser.add_argument('filename')           # positional argument
parser.add_argument('--ports', type=str,  nargs='*',  help='board ports')
parser.add_argument('--labels', type=str,   nargs='*',  help='board  id_label')
parser.add_argument('--added_args', type=str,   nargs='*',  help='added args')
parser.add_argument('--upside_down', type=bool,     help='added args', default=False)

args = parser.parse_args()

print("ports: ", args.ports)
print("labels: ",args.labels)
print("added_args: ", args.added_args)
print("upside_down: " , args.upside_down)

if args.ports and args.labels and len(args.ports) and len(args.labels):
    raise Exception("specify either ports OR labels, not both")

def generate_temp_file(template_file, replacements):
    config = configparser.ConfigParser()
    config.read(template_file)

    for key, value in replacements.items():
        config['main'][key] = str(value)
    print_all_sections(config)
    
    temp_file = tempfile.NamedTemporaryFile(mode='w', delete=False)
    with open(temp_file.name, 'w') as f:
        config.write(f)

    return temp_file.name

def print_all_sections(config):
    for section_name in config.sections():
        print( 'Section:', section_name)
        print ('  Options:', config.options(section_name))
        for name, value in config.items(section_name):
            print( '  %s = %s' % (name, value))

def generate_temp_file2(template_file, replacements):
    with open(template_file, 'r') as f:
        template_content = f.read()

    for key, value in replacements.items():
        placeholder = f"{key}:"
        replace_with = f"{key}: {value}"
        template_content = template_content.replace(placeholder, replace_with)

    temp_file = tempfile.NamedTemporaryFile(mode='w', delete=False)
    temp_file.write(template_content)
    temp_file.close()
    return temp_file.name

def get_element_by_id_label(data_dict, label_value):
    for key, value in data_dict.items():
        if value['id_label'] == label_value:
            return {key: value}
    return None

template_file = "template.conf"

rect = {"left": 3840, "top": 0, "width": 2400, "height": 1650}

class displayData():
    def __init__(self,  port) -> None:
        self.port = port
        self.id_label = None
        self.temp_file = None
        self.ip = None
        self.replacements = None
        self.process = None

d_orders = ["top_left",  "top_right", "bottom_left", "bottom_right"]
ports = []
if not args.ports:
    # board_data =  get_board_ips.read_serial_ports( ["COM13", "COM14", "COM15", "COM16"])
    board_data =  get_board_ips.read_serial_ports( ["COM20", "COM19"])#, "COM18", "COM19", "COM20"])
    # board_data =  get_board_ips.read_serial_ports(["COM13"]) 
    for key, data in board_data.items():
        if  data['id_label'] in args.labels:
            ports.append(key)
else:
    ports = args.ports# ["COM16", "COM17", "COM18", "COM19"]
    
displayDataArr = [displayData(port) for port in ports]

for ddata in displayDataArr:
    for key, data in board_data.items():
        if key == ddata.port:
            ddata.id_label = data["id_label"]
            ddata.ip = data["ip_address"]

if args.upside_down:
    d_orders.reverse()
    
for i, ddata in enumerate(displayDataArr):
#for  key, value in replacements.items():
    n = d_orders.index(ddata.id_label)
    
    ddata.replacements = {
        'ip_address': ddata.ip, #data[next(iter(data))]["ip_adress"],
        'x_offset': rect["left"]+ (n%2)*(rect["width"]//2),
        'y_offset': rect["top"]+ (int(n/2))*(rect["height"]//2),
        'id': n,
        'rotation': (180 if  n <= 1 else 0) if  args.upside_down else (180 if  n >= 1 else 0)
    }
    #n+=1
    ddata.temp_file = generate_temp_file(template_file, ddata.replacements)
    print("Temporary file generated:", ddata.temp_file)
    args_ = ["python", "screen_capture.py", ddata.temp_file] + (args.added_args if args.added_args else [])
    if i != 0: args_+= ["nokeychecker"]
    ddata.process =  subprocess.Popen(args_ )
    #subprocess.Popen(["serial_reader.bat", f'{ddata.port}'], creationflags=subprocess.CREATE_NEW_CONSOLE)


    time.sleep(0.0)
    
for ddata in displayDataArr:
    ddata.process.wait()
print()




