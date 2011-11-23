// Copyright 2011 Prashanth Mundkur.
//
//   This file is part of the CRM114 Library.
//
//   The CRM114 Library is free software: you can redistribute it and/or modify
//   it under the terms of the GNU Lesser General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   The CRM114 Library is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU Lesser General Public License for more details.
//
//   You should have received a copy of the GNU Lesser General Public License
//   along with the CRM114 Library.  If not, see <http://www.gnu.org/licenses/>.

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

/* Objects and Types */

static PyObject *ErrorObject = NULL;

static PyTypeObject CB_Type;
typedef struct {
  PyObject_HEAD
  CRM114_CONTROLBLOCK *p_cb;
} CB_Object;

static PyTypeObject DB_Type;
typedef struct {
  PyObject_HEAD
  CRM114_DATABLOCK *p_db;
} DB_Object;

static PyTypeObject Result_Type;
typedef struct {
  PyObject_HEAD
  CRM114_MATCHRESULT mr;
} Result_Object;

/* Control block */

static PyObject *
CB_new(PyTypeObject *dummy, PyObject *args, PyObject *kwargs) {
  UNUSED(dummy);
  CB_Object *self;
  CRM114_ERR cerr;

  if ((self = (CB_Object *)PyObject_New(CB_Object, &CB_Type)) == NULL)
    return NULL;

  if ((self->p_cb = crm114_new_cb()) == NULL) {
    Py_CLEAR(self);
    return PyErr_NoMemory();
  }

  unsigned long long flags = 0;
  const char *regex = NULL; int regex_len = 0;
  size_t start_mem = 0;
  PyObject *classes = NULL, *class_iter = NULL, *class = NULL; int nclass = 0;
  static char *kwlist[] = {"flags", "classes", "regex", "start_mem", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "KO!|s#n:ControlBlock_new",
                                   kwlist, &flags, &PyList_Type, &classes,
                                   &regex, &regex_len, &start_mem)) {
    Py_CLEAR(self);
    return NULL;
  }

  // Set the classifier.
  if ((cerr = crm114_cb_setflags(self->p_cb, flags)) != CRM114_OK) {
    (void)PyErr_Format(ErrorObject, "error setting control block flags: %s",
                       crm114_strerror(cerr));
    goto error;
  }

  // Set classifier defaults.
  crm114_cb_setclassdefaults(self->p_cb);

  // Set the regex.
  if ((regex_len > 0) &&
      (cerr = crm114_cb_setregex(self->p_cb, regex, regex_len)) != CRM114_OK) {
    PyErr_Format(ErrorObject, "error setting control block regex: %s",
                 crm114_strerror(cerr));
    goto error;
  }

  // Configure the classes.

  if ((class_iter = PyObject_GetIter(classes)) == NULL) {
    PyErr_SetString(ErrorObject, "invalid control block classes: not an iterable");
    Py_CLEAR(classes);
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
    if (!PyArg_ParseTuple(class, "s#O!", &nm, &nm_len, &PyBool_Type, &bool)) {
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

    Py_CLEAR(class);
    continue;

  loop_error:
    Py_CLEAR(class);
    Py_CLEAR(class_iter);
    Py_CLEAR(classes);
    goto error;
  }

  Py_CLEAR(class_iter);
  Py_CLEAR(classes);

  if (nclass == 0) {
    PyErr_SetString(ErrorObject, "invalid control block classes: empty sequence");
    goto error;
  }
  if (class) {
    Py_CLEAR(class);
    PyErr_SetString(ErrorObject, "too many classes specified");
    goto error;
  }
  self->p_cb->how_many_classes = nclass;

  // Set optional starting memory size.
  if (start_mem > 0)
    self->p_cb->datablock_size = start_mem;

  // Finish internal configuration.
  crm114_cb_setblockdefaults (self->p_cb);

  return (PyObject *)self;

 error:
  Py_CLEAR(self);
  return NULL;
}

static void
CB_dealloc(CB_Object *self) {
  crm114_free(self->p_cb);
  Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
CB_dump(CB_Object *self, PyObject *args) {
  PyObject *fpo;
  FILE *fp;
  CRM114_ERR cerr;

  if (!PyArg_ParseTuple(args, "O!", &PyFile_Type, &fpo))
    return NULL;
  fp = PyFile_AsFile(fpo);
  if ((cerr = crm114_cb_write_text_fp(self->p_cb, fp)) != CRM114_OK) {
    PyErr_Format(ErrorObject, "error storing control block: %s", crm114_strerror(cerr));
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject *
CB_load(PyObject *type, PyObject *args) {
  PyObject *fpo;
  CB_Object *self;
  FILE *fp;
  CRM114_CONTROLBLOCK *p_cb;

  if (!PyArg_ParseTuple(args, "O!", &PyFile_Type, &fpo))
    return NULL;
  fp = PyFile_AsFile(fpo);
  if ((p_cb = crm114_cb_read_text_fp(fp)) == NULL) {
    PyErr_Format(ErrorObject, "error reading control block");
    return NULL;
  }
  if ((self = (CB_Object *)PyObject_New(CB_Object, &CB_Type)) == NULL)
    return NULL;
  self->p_cb = p_cb;
  return (PyObject *)self;
}

static PyMethodDef CB_methods[] = {
  {"dump", (PyCFunction)CB_dump, METH_VARARGS,
   "store data block into a file"},
  {"load", (PyCFunction)CB_load, METH_CLASS | METH_VARARGS,
   "load data block from a file"},
  {NULL}                        /* sentinel          */
};

static PyTypeObject CB_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "pycrm114.ControlBlock",      /* tp_name           */
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
  CB_new,                       /* tp_new            */
  0                             /* tp_free           */
};

/* Data blocks */

static PyObject *
DB_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
  UNUSED(kwargs);
  DB_Object *self;
  CB_Object *cb;

  if (!PyArg_ParseTuple(args, "O!|DataBlock_new", &CB_Type, &cb))
    return NULL;
  if (cb->p_cb == NULL) {
    PyErr_SetString(ErrorObject, "no control block data found");
    return NULL;
  }
  if ((self = (DB_Object *)PyObject_New(DB_Object, &DB_Type)) == NULL)
    return NULL;
  if ((self->p_db = crm114_new_db(cb->p_cb)) == NULL) {
    Py_CLEAR(self);
    return PyErr_NoMemory();
  }
  return (PyObject *)self;
}

void DB_dealloc(DB_Object *self) {
  crm114_free(self->p_db);
  Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
DB_learn_text(DB_Object *self, PyObject *args) {
  const char *text; int text_len;
  int nclass;
  CRM114_ERR cerr;

  if (!PyArg_ParseTuple(args, "ns#", &nclass, &text, &text_len))
    return NULL;
  if (nclass >= self->p_db->cb.how_many_classes) {
    PyErr_SetString(ErrorObject, "invalid (out of range) class");
    return NULL;
  }
  if ((cerr = crm114_learn_text(&self->p_db, nclass, text, text_len)) != CRM114_OK) {
    PyErr_Format(ErrorObject, "error learning text: %s", crm114_strerror(cerr));
    return NULL;
  }
  Py_RETURN_NONE;
}

static Result_Object *Result_new(void);
static PyObject *
DB_classify_text(DB_Object *self, PyObject *args) {
  const char *text; int text_len;
  CRM114_ERR cerr;
  Result_Object *result;

  if ((result = Result_new()) == NULL)
    return NULL;

  if (!PyArg_ParseTuple(args, "s#", &text, &text_len))
    return NULL;
  if ((cerr = crm114_classify_text(self->p_db, text, text_len, &(result->mr))) != CRM114_OK) {
    PyErr_Format(ErrorObject, "error classifying text: %s", crm114_strerror(cerr));
    return NULL;
  }
  return (PyObject *)result;
}

static PyObject *
DB_dump(DB_Object *self, PyObject *args) {
  PyObject *fpo;
  FILE *fp;
  CRM114_ERR cerr;

  if (!PyArg_ParseTuple(args, "O!", &PyFile_Type, &fpo))
    return NULL;
  fp = PyFile_AsFile(fpo);
  if ((cerr = crm114_db_write_text_fp(self->p_db, fp)) != CRM114_OK) {
    PyErr_Format(ErrorObject, "error storing data block: %s", crm114_strerror(cerr));
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject *
DB_load(PyObject *type, PyObject *args) {
  PyObject *fpo;
  DB_Object *self;
  FILE *fp;
  CRM114_DATABLOCK *p_db;

  if (!PyArg_ParseTuple(args, "O!", &PyFile_Type, &fpo))
    return NULL;
  fp = PyFile_AsFile(fpo);
  if ((p_db = crm114_db_read_text_fp(fp)) == NULL) {
    PyErr_Format(ErrorObject, "error reading data block");
    return NULL;
  }
  if ((self = (DB_Object *)PyObject_New(DB_Object, &DB_Type)) == NULL)
    return NULL;
  self->p_db = p_db;
  return (PyObject *)self;
}

static PyMethodDef DB_methods[] = {
  {"learn_text", (PyCFunction)DB_learn_text, METH_VARARGS,
   "learn some example text into the specified class"},
  {"classify_text", (PyCFunction)DB_classify_text, METH_VARARGS,
   "classify text into one of the learned classes"},
  {"dump", (PyCFunction)DB_dump, METH_VARARGS,
   "store data block into a file"},
  {"load", (PyCFunction)DB_load, METH_CLASS | METH_VARARGS,
   "load data block from a file"},
  {NULL}                        /* sentinel          */
};

static PyTypeObject DB_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "pycrm114.DataBlock",         /* tp_name           */
  sizeof(DB_Object),            /* tp_basicsize      */
  0,                            /* tp_itemsize       */

  /* Methods */
  (destructor)DB_dealloc,       /* tp_dealloc        */
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

  DB_methods,                   /* tp_methods        */
  0,                            /* tp_members        */
  0,                            /* tp_getset         */
  0,                            /* tp_base           */
  0,                            /* tp_dict           */
  0,                            /* tp_descr_get      */
  0,                            /* tp_descr_set      */
  0,                            /* tp_dictoffset     */
  0,                            /* tp_init           */
  0,                            /* tp_alloc          */
  DB_new,                       /* tp_new            */
  0                             /* tp_free           */
};

/* Match result */

/* This allocator is purely for internal use.  It is not in the
   Result_methods table so that the only way results can be created is
   as results from the classify function.  */
static Result_Object *
Result_new() {
  return (Result_Object *)PyObject_New(Result_Object, &Result_Type);
}

static void
Result_dealloc(Result_Object *self) {
  Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
Result_best_match(Result_Object *self) {
  int idx = self->mr.bestmatch_index;
  return PyString_FromString(self->mr.class[idx].name);
}

static PyObject *
Result_tsprob(Result_Object *self) {
  return PyFloat_FromDouble(self->mr.tsprob);
}

static PyObject *
Result_overall_pR(Result_Object *self) {
  return PyFloat_FromDouble(self->mr.overall_pR);
}

static PyObject *
Result_unk_features(Result_Object *self) {
  return PyInt_FromLong(self->mr.unk_features);
}

static PyObject *
Result_scores(Result_Object *self) {
  int i;
  PyObject *list;

  if ((list = PyList_New(self->mr.how_many_classes)) == NULL)
    return NULL;

  for (i = 0; i < self->mr.how_many_classes; i++) {
    PyObject *d = NULL;
    PyObject *name = NULL, *pR = NULL, *prob = NULL;
    PyObject *docs = NULL, *feats = NULL, *hits = NULL, *succ = NULL;

    if ((d = PyDict_New()) == NULL)
      goto error;

    if ((name = PyString_FromString(self->mr.class[i].name)) == NULL)
      goto loop_error;
    if (PyDict_SetItemString(d, "name", name) < 0)
      goto loop_error;

    if ((pR = PyFloat_FromDouble(self->mr.class[i].pR)) == NULL)
      goto loop_error;
    if (PyDict_SetItemString(d, "pR", pR) < 0)
      goto loop_error;

    if ((prob = PyFloat_FromDouble(self->mr.class[i].prob)) == NULL)
      goto loop_error;
    if (PyDict_SetItemString(d, "prob", prob) < 0)
      goto loop_error;

    if ((docs = PyInt_FromLong(self->mr.class[i].documents)) == NULL)
      goto loop_error;
    if (PyDict_SetItemString(d, "documents", docs) < 0)
      goto loop_error;

    if ((feats = PyInt_FromLong(self->mr.class[i].features)) == NULL)
      goto loop_error;
    if (PyDict_SetItemString(d, "features", feats) < 0)
      goto loop_error;

    if ((hits = PyInt_FromLong(self->mr.class[i].hits)) == NULL)
      goto loop_error;
    if (PyDict_SetItemString(d, "hits", hits) < 0)
      goto loop_error;

    if ((succ = PyInt_FromLong(self->mr.class[i].success)) == NULL)
      goto loop_error;
    if (PyDict_SetItemString(d, "success", succ) < 0)
      goto loop_error;

    PyList_SET_ITEM(list, i, d);
    continue;

  loop_error:
    Py_CLEAR(succ);
    Py_CLEAR(hits);
    Py_CLEAR(feats);
    Py_CLEAR(docs);
    Py_CLEAR(prob);
    Py_CLEAR(pR);
    Py_CLEAR(name);
    Py_CLEAR(d);
    goto error;
  }

  return list;

 error:
  Py_CLEAR(list);
  return NULL;
}

static PyMethodDef Result_methods[] = {
  {"best_match", (PyCFunction)Result_best_match, METH_NOARGS,
   "name of best matching class"},
  {"tsprob", (PyCFunction)Result_tsprob, METH_NOARGS,
   "total success probability"},
  {"overall_pR", (PyCFunction)Result_overall_pR, METH_NOARGS,
   "overall_pR"},
  {"unk_features", (PyCFunction)Result_unk_features, METH_NOARGS,
   "unknown features"},
  {"scores", (PyCFunction)Result_scores, METH_NOARGS,
   "match scores for each class"},
  {NULL}                        /* sentinel          */
};

static PyTypeObject Result_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "pycrm114.MatchResult",       /* tp_name           */
  sizeof(Result_Object),        /* tp_basicsize      */
  0,                            /* tp_itemsize       */

  /* Methods */
  (destructor)Result_dealloc,   /* tp_dealloc        */
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

  Result_methods,               /* tp_methods        */
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

/* Module initialization */

static char module_doc [] =
  "This module implements an interface to the libcrm114 library.\n";

static PyMethodDef crm114_methods[] = {
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
  Py_CLEAR(key);
  Py_CLEAR(value);
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
initpycrm114(void) {
  PyObject *m, *d;

  /* Create the module. */
  m = Py_InitModule3("pycrm114", crm114_methods, module_doc);
  assert(m != NULL && PyModule_Check(m));

  /* Add module objects. */
  if (PyType_Ready(&CB_Type) < 0)
    return;
  Py_INCREF(&CB_Type);
  PyModule_AddObject(m, "ControlBlock", (PyObject *)&CB_Type);
  if (PyType_Ready(&DB_Type) < 0)
    return;
  Py_INCREF(&DB_Type);
  PyModule_AddObject(m, "DataBlock", (PyObject *)&DB_Type);

  if (PyType_Ready(&Result_Type) < 0)
    return;
  Py_INCREF(&Result_Type);
  PyModule_AddObject(m, "MatchResult", (PyObject *)&Result_Type);

  /* Get module dict to populate it. */
  d = PyModule_GetDict(m);
  assert(d != NULL);

  /* Add error object. */
  ErrorObject = PyErr_NewException("pycrm114.error", NULL, NULL);
  assert(ErrorObject != NULL);
  PyDict_SetItemString(d, "error",   ErrorObject);

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

}
