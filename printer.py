import gdb.printing

def walk_chain(node):
    while node != 0:
        entry = node.dereference()
        key = entry['key'].string()
        value = int(entry['value'])
        yield (key, value)
        node = entry['next']

class TablePrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        count = int(self.val['count'])
        size = int(self.val['size'])
        return f"table(count={count}, size={size})"

    def children(self):
        size = int(self.val['size'])
        buckets = self.val['buckets']
        for i in range(size):
            for key, value in walk_chain(buckets[i]):
                yield ("key", key)
                yield ("value", value)

    def display_hint(self):
        return 'map'

class DumpTable(gdb.Command):
    def __init__(self):
        super().__init__("dump-table", gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        if not arg:
            raise gdb.GdbError("usage: dump-table <table expr>")

        val = gdb.parse_and_eval(arg)
        if val.type.code == gdb.TYPE_CODE_PTR:
            val = val.dereference()

        size = int(val['size'])
        buckets = val['buckets']

        for i in range(size):
            res = [f"{key}={value}" for key, value in walk_chain(buckets[i])]

            if not res:
                print(f"bucket {i}: (empty)")
            else:
                chain = " -> ".join(res)
                print(f"bucket {i}, length: {len(res)}, list: {chain}")

DumpTable()
pp = gdb.printing.RegexpCollectionPrettyPrinter("hashtable")
pp.add_printer("table", "^table$", TablePrinter)
gdb.printing.register_pretty_printer(gdb.current_objfile(), pp)