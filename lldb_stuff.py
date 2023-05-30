import lldb
import os



def string_view_summary(valobj, internal_dict):
    length = valobj.GetChildMemberWithName('length').GetValueAsUnsigned(0)
    if length == 0: return '""'
    truncated = False
    if length > 1024: 
        length = 1024
        truncated = True
    ptr = valobj.GetChildMemberWithName('text').GetValueAsUnsigned(0)
    if ptr == 0: 
        return '(nil)'
    err = lldb.SBError()
    buff = valobj.process.ReadMemory(ptr, length, err)
    if err.Fail(): 
        return f'<error: {err}>'
    result = buff.decode(encoding='utf-8', errors='ignore')
    if truncated:
        return f'{repr(result)}(truncated)'
    return f'{repr(result)}'


def __lldb_init_module(debugger, internal_dict):
    modname = os.path.splitext(os.path.basename(__file__))[0]
    debugger.HandleCommand(f'type summary add StringView -F {modname}.string_view_summary')
