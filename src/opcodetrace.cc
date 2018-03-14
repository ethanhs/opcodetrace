#include "Python.h"
#include <frameobject.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif

static int counts[256] = {0};

// The main frame hook, hook into Python and count bytecode
PyObject *OpcodeTracer_EvalFrame(PyFrameObject *frame, int throwflag)
{
    if (!throwflag)
    {
        PyCodeObject *code = frame->f_code;
        Py_ssize_t len = PySequence_Length(code->co_code);
        for (Py_ssize_t i = 0; i < len; i += 2)
        {
            PyObject *op = PySequence_GetItem(code->co_code, i);
            long opcode = PyLong_AsLong(op);
            counts[opcode] += 1;
        }
        return _PyEval_EvalFrameDefault(frame, throwflag);
    }
    return _PyEval_EvalFrameDefault(frame, throwflag);
}

static PyObject *OpcodeTracer_UnHook(PyObject *self, PyObject* args)
{
    PyObject* path = NULL;
    const char *c_path = "opcode_trace.log";
    if (PyArg_ParseTuple(args, "s", &path)) {
        c_path = PyUnicode_AsUTF8(path);
    }
    FILE *f = fopen(c_path, "a");
    fprintf(f, "(");
    for (int i = 0; i < 164; i++)
    {
        fprintf(f, "%d,", counts[i]);
    }
    fprintf(f, ")");
    fclose(f);
    PyThreadState *state = PyThreadState_Get();
    _PyFrameEvalFunction func = state->interp->eval_frame;
    state->interp->eval_frame = _PyEval_EvalFrameDefault;
    if (func == OpcodeTracer_EvalFrame)
    {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject *OpcodeTracer_Hook(PyObject *self)
{
    PyThreadState *state = PyThreadState_Get();
    _PyFrameEvalFunction func = state->interp->eval_frame;
    state->interp->eval_frame = OpcodeTracer_EvalFrame;
    if (func == OpcodeTracer_EvalFrame)
    {
        Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}

static PyObject *OpcodeTracer_Hooked(PyObject *self)
{
    PyThreadState *state = PyThreadState_Get();
    _PyFrameEvalFunction func = state->interp->eval_frame;
    if (func == OpcodeTracer_EvalFrame)
    {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyMethodDef OpcodeTracerMethods[] = {
    {"hook",
     (PyCFunction)OpcodeTracer_Hook,
     METH_NOARGS,
     "Hook into the interpreter."},
    {"unhook",
     (PyCFunction)OpcodeTracer_UnHook,
     METH_VARARGS,
     "Remove the hook from the interpreter."},
    {"hooked",
     (PyCFunction)OpcodeTracer_Hooked,
     METH_NOARGS,
     "Return True if hooked, False otherwise."},
    {NULL, NULL, 0, NULL} /* sentinal node, do not delete */
};

static struct PyModuleDef OpcodeTracerModule = {
    PyModuleDef_HEAD_INIT,
    "opcodetracer",
    "OpcodeTracer, a module to measure the number of opcodes in code.",
    -1,
    OpcodeTracerMethods};

PyMODINIT_FUNC PyInit_opcodetrace(void)
{
    return PyModule_Create(&OpcodeTracerModule);
}

#ifdef __cplusplus
}
#endif