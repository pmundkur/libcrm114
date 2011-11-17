#include <Python.h>
#include <assert.h>
#include <string.h>

#include "crm114_sysincludes.h"
#include "crm114_config.h"
#include "crm114_structs.h"
#include "crm114_lib.h"
#include "crm114_internal.h"

#undef UNUSED
#define UNUSED(var)     ((void)&var)

typedef struct {
  PyObject_HEAD
  CRM114_CONTROLBLOCK *p_cb;
} CB_Object;

static PyObject *ErrorObject = NULL;

/*
  ControlBlock:
      [ pipeline ]
  DataBlock:
  - init(ControlBlock)
  - learn_text(class_name, "text")
  - size()
  - save_binary(file)
  - load_binary(file)
  - classify("text") -> Result

  ResultBlock:
  - no init
  - bunch of getters
  - __str__() or show()
 */

static PyTypeObject CB_Type;
static PyObject *
CB_new(PyObject *dummy, PyObject *args, PyObject *kwargs) {
  UNUSED(dummy);
  CB_Object *self;

  if ((self = (CB_Object *)PyObject_New(CB_Object, &CB_Type)) == NULL)
    return NULL;

  if ((self->p_cb = crm114_new_cb()) == NULL) {
    Py_DECREF(self);
    return PyErr_NoMemory();
  }

  unsigned long long flags = 0;
  const char *regex = NULL; int regex_len = 0;
  size_t start_mem = 0;
  PyObject *classes = NULL, *class_iter = NULL, *class = NULL; int nclass = 0;
  static char *kwlist[] = {"flags", "regex", "classes", "start_mem", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Ks#|O!n:ControlBlock_new", kwlist,
                                   &flags, &regex, &regex_len,
                                   PyTuple_Type, &classes, &start_mem)) {
    Py_DECREF(self);
    return NULL;
  }
  // Set the classifier.
  if (crm114_cb_setflags(self->p_cb, flags) != CRM114_OK) {
    // TODO: convert crm114 error code into useful string
    PyErr_SetString(ErrorObject, "error setting control block flags");
    goto error;
  }
  // Set classifier defaults.
  crm114_cb_setclassdefaults(self->p_cb);
  // Set the regex.
  if (crm114_cb_setregex(self->p_cb, regex, regex_len) != CRM114_OK) {
    PyErr_SetString(ErrorObject, "error setting control block regex");
    goto error;
  }
  // Configure the optional classes.
  if (classes) {
    // It is not too late to increment here, since we haven't
    // performed any Python allocation yet.
    Py_INCREF(classes);

    if ((class_iter = PyObject_GetIter(classes)) == NULL) {
      PyErr_SetString(ErrorObject, "invalid control block classes: not an iterable");
      Py_DECREF(classes);
      goto error;
    }

    for (nclass = 0;
         (class = PyIter_Next(class_iter)) && nclass < CRM114_MAX_CLASSES;
         nclass++) {

      const char *nm = NULL; int nm_len = 0;
      PyObject *bool = NULL;

      if (!PyTuple_Check(class)) {
        PyErr_SetString(ErrorObject, "invalid control block class: not a tuple");
        goto loop_error;
      }
      if (!PyArg_ParseTuple(class, "s#O!", &nm, &nm_len, PyBool_Type, &bool)) {
        PyErr_SetString(ErrorObject, "invalid control block class: invalid tuple");
        goto loop_error;
      }
      if (nm_len > CLASSNAME_LENGTH) {
        PyErr_SetString(ErrorObject, "invalid control block class: class name too long");
        goto loop_error;
      }

      // We depend on the memset in crm114_new_cb() to ensure NUL-termination.
      strncpy(self->p_cb->class[nclass].name, nm, CLASSNAME_LENGTH);
      self->p_cb->class[nclass].success = (bool == Py_True);

      Py_DECREF(class);
      continue;

    loop_error:
      Py_DECREF(class);
      Py_DECREF(class_iter);
      Py_DECREF(classes);
      goto error;
    }

    Py_DECREF(class_iter);
    Py_DECREF(classes);

    if (nclass == 0) {
      PyErr_SetString(ErrorObject, "invalid control block classes: empty sequence");
      goto error;
    }
  }
  // Set optional starting memory size.
  if (start_mem > 0)
    self->p_cb->datablock_size = start_mem;
  // Finish internal configuration.
  crm114_cb_setblockdefaults (self->p_cb);

  return (PyObject *)self;

 error:
  Py_DECREF(self);
  return NULL;
}

static void
CB_dealloc(CB_Object *self) {
  crm114_free(self->p_cb);
  Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyMethodDef CB_methods[] = {
  {NULL}                        /* Sentinel          */
};

static PyTypeObject CB_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "ControlBlock",               /* tp_name           */
  sizeof(CB_Object),            /* tp_basicsize      */
  0,                            /* tp_itemsize       */

  /* Methods */
  (destructor)CB_dealloc,       /* tp_dealloc        */
  0,                            /* tp_print          */
  0,                            /* tp_getattr        */
  0,                            /* tp_setattr        */
  0,                            /* tp_compare        */
  0,                            /* tp_repr           */

  0,                            /* tp_as_number      */
  0,                            /* tp_as_sequence    */
  0,                            /* tp_as_mapping     */

  0,                            /* tp_hash           */
  0,                            /* tp_call           */
  0,                            /* tp_str            */
  0,                            /* tp_getattro       */
  0,                            /* tp_setattro       */

  0,                            /* tp_as_buffer      */
  Py_TPFLAGS_DEFAULT,           /* tp_flags          */
  0,                            /* tp_doc            */
  0,                            /* tp_traverse       */
  0,                            /* tp_clear          */
  0,                            /* tp_richcompare    */
  0,                            /* tp_weaklistoffset */
  0,                            /* tp_iter           */
  0,                            /* tp_iternext       */

  CB_methods,                   /* tp_methods        */
  0,                            /* tp_members        */
  0,                            /* tp_getset         */
  0,                            /* tp_base           */
  0,                            /* tp_dict           */
  0,                            /* tp_descr_get      */
  0,                            /* tp_descr_set      */
  0,                            /* tp_dictoffset     */
  0,                            /* tp_init           */
  0,                            /* tp_alloc          */
  0,                            /* tp_new            */
  0                             /* tp_free           */
};

static char module_doc [] =
  "This module implements an interface to the libcrm114 library.\n";

static PyMethodDef crm114_methods[] = {
  {"ControlBlock", (PyCFunction) CB_new, METH_VARARGS | METH_KEYWORDS, NULL},
  {NULL, NULL, 0, NULL}
};

static void
insobj(PyObject *d, char *name, PyObject *value) {
  PyObject *key = NULL;
  if (d == NULL || value == NULL)
    goto error;
  key = PyString_FromString(name);
  if (key == NULL)
    goto error;

  assert(PyDict_GetItem(d, key) == NULL);
  if (PyDict_SetItem(d, key, value) != 0)
    goto error;
  Py_DECREF(key);
  Py_DECREF(value);
  return;

 error:
  Py_FatalError("pycrm114: insobj() failed");
  assert(0);
}

static void
inslong(PyObject *d, char *name, unsigned long long value) {
  PyObject *v = PyInt_FromLong(value);
  insobj(d, name, v);
}

PyMODINIT_FUNC
init_pycrm114_module(void) {
  PyObject *m, *d;

  /* Create the module. */
  m = Py_InitModule3("pycrm114", crm114_methods, module_doc);
  assert(m != NULL && PyModule_Check(m));
  d = PyModule_GetDict(m);
  assert(d != NULL);

  /* Add error object. */
  ErrorObject = PyErr_NewException("pycrm114.error", NULL, NULL);
  assert(ErrorObject != NULL);
  PyDict_SetItemString(d, "error", ErrorObject);

  /* Add flags. */

  inslong(d, "CRM114_BYCHAR", CRM114_BYCHAR);
  inslong(d, "CRM114_STRING", CRM114_STRING);

  inslong(d, "CRM114_REFUTE", CRM114_REFUTE);
  inslong(d, "CRM114_MICROGROOM", CRM114_MICROGROOM);
  inslong(d, "CRM114_MARKOVIAN", CRM114_MARKOVIAN);
  inslong(d, "CRM114_OSB_BAYES", CRM114_OSB_BAYES);
  inslong(d, "CRM114_OSB", CRM114_OSB);
  inslong(d, "CRM114_CORRELATE", CRM114_CORRELATE);
  inslong(d, "CRM114_OSB_WINNOW", CRM114_OSB_WINNOW);
  inslong(d, "CRM114_WINNOW", CRM114_WINNOW);
  inslong(d, "CRM114_CHI2", CRM114_CHI2);
  inslong(d, "CRM114_UNIQUE", CRM114_UNIQUE);
  inslong(d, "CRM114_ENTROPY", CRM114_ENTROPY);
  inslong(d, "CRM114_OSBF", CRM114_OSBF);
  inslong(d, "CRM114_OSBF_BAYES", CRM114_OSBF_BAYES);
  inslong(d, "CRM114_HYPERSPACE", CRM114_HYPERSPACE);
  inslong(d, "CRM114_UNIGRAM", CRM114_UNIGRAM);
  inslong(d, "CRM114_CROSSLINK", CRM114_CROSSLINK);

  inslong(d, "CRM114_READLINE", CRM114_READLINE);
  inslong(d, "CRM114_DEFAULT", CRM114_DEFAULT);
  inslong(d, "CRM114_SVM", CRM114_SVM);
  inslong(d, "CRM114_FSCM", CRM114_FSCM);
  inslong(d, "CRM114_NEURAL_NET", CRM114_NEURAL_NET);
  inslong(d, "CRM114_ERASE", CRM114_ERASE);
  inslong(d, "CRM114_PCA", CRM114_PCA);
  inslong(d, "CRM114_BOOST", CRM114_BOOST);

  //  inslong(d, "", );

}
