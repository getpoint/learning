from ctypes import *

class Py_A(Structure):
    _fields_ = [('py_int', c_int),
                ('py_list', c_char * 32)]

class Py_B(Structure):
    _fields_ = [('py_a', Py_A),
                ('py_int', c_int),
                ('py_list', c_char * 32)]

TESTFUNC = CFUNCTYPE(POINTER(c_int), POINTER(c_char), POINTER(Py_B))

def test(sig_no):
    clib = cdll.LoadLibrary('./libccalled.so')
    py_b = Py_B()
    py_i = c_int(0)
    py_b.py_int = 0
    py_b.py_int = 0
    py_list = create_string_buffer('*' * 31)
    py_b.py_list = '*' * 32
    py_b.py_a.py_list = '*' * 32
    clib.c_func(byref(py_i), py_list, byref(py_b))
    print "i: ", py_i
    print "py_list: ", repr(py_list.raw)
    print "py_b.py_int: ", py_b.py_int
    print "py_b.py_list: ", py_b.py_list
    print "py_b.py_a.py_int: ", py_b.py_a.py_int
    print "py_b.py_a.py_list: ", py_b.py_a.py_list

test(1)
