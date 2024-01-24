import sys
import network
import cross.network

def get_msgs():
	return ["NETMSG_INVALID"] + [m.enum_name for m in network.Messages]

def get_msgs_6():
	return ["NETMSG_INVALID"] + [m.enum_name for m in cross.network.Messages]

def get_objs():
	return ["NETOBJ_INVALID"] + [m.enum_name for m in network.Objects]

def get_objs_6():
	return ["NETOBJ_INVALID"] + [m.enum_name for m in cross.network.Objects]

def generate_map(a, b):
	result = []
	for m in a:
		try:
			result += [b.index(m)]
		except ValueError:
			result += [-1]

	return result

def output_map_header(name, m):
	print(f"extern const int gs_{name}[{len(m)}];")
	print(f"inline int {name}(int a) {{ if(a < 0 || a >= {len(m)}) return -1; return gs_{name}[a]; }}")

def output_map_source(name, m):
	print(f"const int gs_{name}[{len(m)}] = {{")
	print(*m, sep=',')
	print("};")

def main():
	map_header = "map_header" in sys.argv
	map_source = "map_source" in sys.argv
	guard = "GAME_GENERATED_PROTOCOLGLUE"
	if map_header:
		print("#ifndef " + guard)
		print("#define " + guard)
	elif map_source:
		print("#include \"protocolglue.h\"")

	msgs = get_msgs()
	msgs6 = get_msgs_6()

	map7to6 = generate_map(msgs, msgs6)
	map6to7 = generate_map(msgs6, msgs)

	if map_header:
		output_map_header("Msg_SixToSeven", map6to7)
		output_map_header("Msg_SevenToSix", map7to6)
	elif map_source:
		output_map_source("Msg_SixToSeven", map6to7)
		output_map_source("Msg_SevenToSix", map7to6)

	objs = get_objs()
	objs6 = get_objs_6()

	objs7to6 = generate_map(objs, objs6)
	objs6to7 = generate_map(objs6, objs)

	if map_header:
		output_map_header("Obj_SixToSeven", objs6to7)
		output_map_header("Obj_SevenToSix", objs7to6)
		print("#endif //" + guard)
	elif map_source:
		output_map_source("Obj_SixToSeven", objs6to7)
		output_map_source("Obj_SevenToSix", objs7to6)


if __name__ == "__main__":
	main()
