import gdb.printing

class MaxSizeParam(gdb.Parameter):
    """Upper bound on table size the hashtable printer treats as valid"""
    set_doc = "Set the max table size the printer trusts"
    show_doc = "Show the max table size the printer trusts"
    def __init__(self):
        super().__init__("hashtable-max-size", gdb.COMMAND_DATA, gdb.PARAM_ZUINTEGER)
        # Sanity cap, adjustable at runtime
        self.value = 1_000_000

max_size = MaxSizeParam()

def walk_chain(node, limit):
    steps = 0
    try:
        while node != 0:
            if steps >= limit:
                return
            entry = node.dereference()
            yield entry
            node = entry['next']
            steps += 1
    except gdb.MemoryError:
        return

def is_table_sane(val):
    try:
        return 0 < int(val['size']) <= max_size.value and int(val['count']) <= max_size.value
    except gdb.MemoryError:
        return False

class TablePrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            count = int(self.val['count'])
            size = int(self.val['size'])
        except gdb.MemoryError:
            return "table(<unreadable>)"

        if not is_table_sane(self.val):
            return f"table(invalid: size={size}, count={count})"
    
        return f"table(count={count}, size={size})"

    def children(self):
        if not is_table_sane(self.val):
            return

        size = int(self.val['size'])
        count = int(self.val['count'])
        buckets = self.val['buckets']

        total = 0
        for i in range(size):
            for entry in walk_chain(buckets[i], count):
                if total >= count:
                    return
                yield ("key", entry['key'].lazy_string())
                yield ("value", entry['value'])
                total += 1

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

        if val.type.strip_typedefs().tag != "table":
            raise gdb.GdbError("dump-table: expected a struct table")

        if not is_table_sane(val):
            raise gdb.GdbError("table looks uninitialized or corrupted")

        size = int(val['size'])
        count = int(val['count'])
        buckets = val['buckets']

        total = 0
        for i in range(size):
            try:
                res = [f"{entry['key'].string(errors='replace')}={int(entry['value'])}" for entry in walk_chain(buckets[i], count - total)]
            except gdb.MemoryError:
                print(f"bucket {i}: <unreadable>")
                continue

            total += len(res)

            if not res:
                print(f"bucket {i}: (empty)")
            else:
                chain = " -> ".join(res)
                print(f"bucket {i}, length: {len(res)}, list: {chain}")

DumpTable()
pp = gdb.printing.RegexpCollectionPrettyPrinter("hashtable")
pp.add_printer("table", "^table$", TablePrinter)
gdb.printing.register_pretty_printer(gdb.current_objfile(), pp, replace=True)