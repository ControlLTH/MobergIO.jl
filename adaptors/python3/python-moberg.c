/*
    python-moberg.c -- python interface to moberg I/O system

    Copyright (C) 2019 Anders Blomdell <anders.blomdell@gmail.com>

    This file is part of Moberg.

    Moberg is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <Python.h>
#include <structmember.h>
#include <moberg.h>

#ifndef Py_UNUSED	/* This is already defined for Python 3.4 onwards */
#ifdef __GNUC__
#define Py_UNUSED(name) _unused_ ## name __attribute__((unused))
#else
#define Py_UNUSED(name) _unused_ ## name
#endif
#endif

static PyTypeObject MobergType;
static PyTypeObject MobergAnalogInType;
static PyTypeObject MobergAnalogOutType;
static PyTypeObject MobergDigitalInType;
static PyTypeObject MobergDigitalOutType;
static PyTypeObject MobergEncoderInType;

/*
 * moberg.Moberg class
 */

typedef struct {
  PyObject_HEAD
  struct moberg *moberg;
} MobergObject;

static void
Moberg_dealloc(MobergObject *self)
{
  if (self->moberg) {
    moberg_free(self->moberg);
  }
  Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *
Moberg_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  static char *kwlist[] = { NULL };
  if (! PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist)) {
    return NULL;
  }
  MobergObject *self = (MobergObject *) type->tp_alloc(type, 0);
  return (PyObject *) self;
}

static int
Moberg_init(MobergObject *self, PyObject *args, PyObject *kwds)
{
  static char *kwlist[] = { NULL };
  if (! PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist)) {
    return -1;
  }
  self->moberg = moberg_new();
  if (! self->moberg) {
    PyErr_SetString(PyExc_OSError, "moberg.Moberg.__init__() failed");
    return -1;
  }
  return 0;
}

static PyObject *
Moberg_analog_in(MobergObject *self, PyObject *args)
{
  int index;
  if (! PyArg_ParseTuple(args, "i", &index)) {
    PyErr_SetString(PyExc_AttributeError, "index");
    return NULL;
  }
  PyObject *ain_args = Py_BuildValue("Oi", self, index);
  PyObject *ain = MobergAnalogInType.tp_new(&MobergAnalogInType,
                                            ain_args, NULL);
  /* NB: _AnalogIn.__init__ should never be called */
  Py_DECREF(ain_args);
  return ain;
}

static PyObject *
Moberg_analog_out(MobergObject *self, PyObject *args)
{
  int index;
  if (! PyArg_ParseTuple(args, "i", &index)) {
    PyErr_SetString(PyExc_AttributeError, "index");
    return NULL;
  }
  PyObject *aout_args = Py_BuildValue("Oi", self, index);
  PyObject *aout = MobergAnalogOutType.tp_new(&MobergAnalogOutType,
                                              aout_args, NULL);
  /* NB: _AnalogOut.__init__ should never be called */
  Py_DECREF(aout_args);
  return aout;
}

static PyObject *
Moberg_digital_in(MobergObject *self, PyObject *args)
{
  int index;
  if (! PyArg_ParseTuple(args, "i", &index)) {
    PyErr_SetString(PyExc_AttributeError, "index");
    return NULL;
  }
  PyObject *din_args = Py_BuildValue("Oi", self, index);
  PyObject *din = MobergDigitalInType.tp_new(&MobergDigitalInType,
                                            din_args, NULL);
  /* NB: _DigitalIn.__init__ should never be called */
  Py_DECREF(din_args);
  return din;
}

static PyObject *
Moberg_digital_out(MobergObject *self, PyObject *args)
{
  int index;
  if (! PyArg_ParseTuple(args, "i", &index)) {
    PyErr_SetString(PyExc_AttributeError, "index");
    return NULL;
  }
  PyObject *dout_args = Py_BuildValue("Oi", self, index);
  PyObject *dout = MobergDigitalOutType.tp_new(&MobergDigitalOutType,
                                              dout_args, NULL);
  /* NB: _DigitalOut.__init__ should never be called */
  Py_DECREF(dout_args);
  return dout;
}

static PyObject *
Moberg_encoder_in(MobergObject *self, PyObject *args)
{
  int index;
  if (! PyArg_ParseTuple(args, "i", &index)) {
    PyErr_SetString(PyExc_AttributeError, "index");
    return NULL;
  }
  PyObject *ein_args = Py_BuildValue("Oi", self, index);
  PyObject *ein = MobergEncoderInType.tp_new(&MobergEncoderInType,
                                             ein_args, NULL);
  /* NB: _EncoderIn.__init__ should never be called */
  Py_DECREF(ein_args);
  return ein;
}

static PyMethodDef Moberg_methods[] = {
    {"analog_in", (PyCFunction) Moberg_analog_in, METH_VARARGS,
     "Return AnalogIn object for channel"
    },
    {"analog_out", (PyCFunction) Moberg_analog_out, METH_VARARGS,
     "Return AnalogOut object for channel"
    },
    {"digital_in", (PyCFunction) Moberg_digital_in, METH_VARARGS,
     "Return DigitalIn object for channel"
    },
    {"digital_out", (PyCFunction) Moberg_digital_out, METH_VARARGS,
     "Return DigitalOut object for channel"
    },
    {"encoder_in", (PyCFunction) Moberg_encoder_in, METH_VARARGS,
     "Return EncoderIn object for channel"
    },

    {NULL}  /* Sentinel */
};

static PyTypeObject MobergType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "moberg.Moberg",
    .tp_doc = "Moberg objects",
    .tp_basicsize = sizeof(MobergObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = Moberg_new,
    .tp_init = (initproc) Moberg_init,
    .tp_dealloc = (destructor) Moberg_dealloc,
    .tp_methods = Moberg_methods,
};

/*
 * moberg._AnalogIn class (should never be directly instatiated) 
 */

typedef struct {
  PyObject_HEAD
  PyObject *moberg_object;
  int index;
  struct moberg_analog_in channel;
} MobergAnalogInObject;

static void
MobergAnalogIn_dealloc(MobergAnalogInObject *self)
{
  if (self->moberg_object) {
    struct moberg *moberg = ((MobergObject*)self->moberg_object)->moberg;
    struct moberg_status status =
      moberg_analog_in_close(moberg, self->index, self->channel);
    if (! moberg_OK(status)) {
      fprintf(stderr, "Failed to close moberg._AnalogIn(%d) [errno=%d]\n",
              self->index, status.result);
    }
    Py_DECREF(self->moberg_object);
  }
  Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *
MobergAnalogIn_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  PyObject *moberg_object;
  int index;
  struct moberg_analog_in channel = {0};
  
  static char *kwlist[] = { "moberg", "index", NULL };
  if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi", kwlist,
                                    &moberg_object, &index)) {
    goto err;
  }
  if (Py_TYPE(moberg_object) != &MobergType) {
    PyErr_SetString(PyExc_AttributeError, "moberg argument is not Moberg");
    goto err;
  }
  struct moberg *moberg = ((MobergObject*)moberg_object)->moberg;
  struct moberg_status status = moberg_analog_in_open(moberg, index, &channel);
  if (! moberg_OK(status)) {
    PyErr_Format(PyExc_OSError, "moberg._AnalogIn(%d) failed with %d",
                 index, status.result);
    goto err;
  }
  MobergAnalogInObject *self;
  self = (MobergAnalogInObject *) type->tp_alloc(type, 0);
  if (self == NULL) { goto close; }
  Py_INCREF(moberg_object);
  self->moberg_object = moberg_object;
  self->index = index;
  self->channel = channel;
  return (PyObject *) self;
close:
  status = moberg_analog_in_close(moberg, index, channel);
err:
  return NULL;
}

static int
MobergAnalogIn_init(MobergAnalogInObject *self, PyObject *args, PyObject *kwds)
{
  PyErr_SetString(PyExc_AttributeError, "Not intended to be called");
  return -1;
}

static PyObject *
MobergAnalogIn_read(MobergAnalogInObject *self, PyObject *Py_UNUSED(ignored))
{
  double value;
  struct moberg_status status = self->channel.read(self->channel.context,
                                                   &value);
  if (!moberg_OK(status)) {
    PyErr_Format(PyExc_OSError, "moberg._AnalogIn(%d).read() failed with %d",
                 self->index, status.result);
  }
  return Py_BuildValue("d", value);
}

static PyMethodDef MobergAnalogIn_methods[] = {
    {"read", (PyCFunction) MobergAnalogIn_read, METH_NOARGS,
     "Sample and return the AnalogIn value"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject MobergAnalogInType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "moberg.AnalogIn",
    .tp_doc = "AnalogIn objects",
    .tp_basicsize = sizeof(MobergAnalogInObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = MobergAnalogIn_new,
    .tp_init = (initproc) MobergAnalogIn_init,
    .tp_dealloc = (destructor) MobergAnalogIn_dealloc,
    .tp_methods = MobergAnalogIn_methods,
};

/*
 * moberg._AnalogOut class (should never be directly instatiated) 
 */

typedef struct {
  PyObject_HEAD
  PyObject *moberg_object;
  int index;
  struct moberg_analog_out channel;
} MobergAnalogOutObject;

static void
MobergAnalogOut_dealloc(MobergAnalogOutObject *self)
{
  if (self->moberg_object) {
    struct moberg *moberg = ((MobergObject*)self->moberg_object)->moberg;
    struct moberg_status status =
      moberg_analog_out_close(moberg, self->index, self->channel);
    if (! moberg_OK(status)) {
      fprintf(stderr, "Failed to close moberg._AnalogOut(%d) [errno=%d]\n",
              self->index, status.result);
    }
    Py_DECREF(self->moberg_object);
  }
  Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *
MobergAnalogOut_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  PyObject *moberg_object;
  int index;
  struct moberg_analog_out channel = {0};
  
  static char *kwlist[] = { "moberg", "index", NULL };
  if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi", kwlist,
                                    &moberg_object, &index)) {
    goto err;
  }
  if (Py_TYPE(moberg_object) != &MobergType) {
    PyErr_SetString(PyExc_AttributeError, "moberg argument is not Moberg");
    goto err;
  }
  struct moberg *moberg = ((MobergObject*)moberg_object)->moberg;
  struct moberg_status status = moberg_analog_out_open(moberg, index, &channel);
  if (! moberg_OK(status)) {
    PyErr_Format(PyExc_OSError, "moberg._AnalogOut(%d) failed with %d",
                 index, status.result);
    goto err;
  }
  MobergAnalogOutObject *self;
  self = (MobergAnalogOutObject *) type->tp_alloc(type, 0);
  if (self == NULL) { goto close; }
  Py_INCREF(moberg_object);
  self->moberg_object = moberg_object;
  self->index = index;
  self->channel = channel;
  return (PyObject *) self;
close:
  status = moberg_analog_out_close(moberg, index, channel);
err:
  return NULL;
}

static int
MobergAnalogOut_init(MobergAnalogOutObject *self, PyObject *args, PyObject *kwds)
{
  PyErr_SetString(PyExc_AttributeError, "Not intended to be called");
  return -1;
}

static PyObject *
MobergAnalogOut_write(MobergAnalogOutObject *self, PyObject *args)
{
  double desired_value, actual_value;
  if (! PyArg_ParseTuple(args, "d", &desired_value)) {
    goto err;
  }
  struct moberg_status status = self->channel.write(self->channel.context,
                                                    desired_value,
                                                    &actual_value);
  if (!moberg_OK(status)) {
    PyErr_Format(PyExc_OSError, "moberg._AnalogOut(%d).write() failed with %d",
                 self->index, status.result);
    goto err;
  }
  
  return Py_BuildValue("d", actual_value);
err:
  return NULL;
}

static PyMethodDef MobergAnalogOut_methods[] = {
    {"write", (PyCFunction) MobergAnalogOut_write, METH_VARARGS,
     "Set AnalogOut value"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject MobergAnalogOutType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "moberg.AnalogOut",
    .tp_doc = "AnalogOut objects",
    .tp_basicsize = sizeof(MobergAnalogOutObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = MobergAnalogOut_new,
    .tp_init = (initproc) MobergAnalogOut_init,
    .tp_dealloc = (destructor) MobergAnalogOut_dealloc,
    .tp_methods = MobergAnalogOut_methods,
};

/*
 * moberg._DigitalIn class (should never be directly instatiated) 
 */

typedef struct {
  PyObject_HEAD
  PyObject *moberg_object;
  int index;
  struct moberg_digital_in channel;
} MobergDigitalInObject;

static void
MobergDigitalIn_dealloc(MobergDigitalInObject *self)
{
  if (self->moberg_object) {
    struct moberg *moberg = ((MobergObject*)self->moberg_object)->moberg;
    struct moberg_status status =
      moberg_digital_in_close(moberg, self->index, self->channel);
    if (! moberg_OK(status)) {
      fprintf(stderr, "Failed to close moberg._DigitalIn(%d) [errno=%d]\n",
              self->index, status.result);
    }
    Py_DECREF(self->moberg_object);
  }
  Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *
MobergDigitalIn_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  PyObject *moberg_object;
  int index;
  struct moberg_digital_in channel = {0};
  
  static char *kwlist[] = { "moberg", "index", NULL };
  if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi", kwlist,
                                    &moberg_object, &index)) {
    goto err;
  }
  if (Py_TYPE(moberg_object) != &MobergType) {
    PyErr_SetString(PyExc_AttributeError, "moberg argument is not Moberg");
    goto err;
  }
  struct moberg *moberg = ((MobergObject*)moberg_object)->moberg;
  struct moberg_status status = moberg_digital_in_open(moberg, index, &channel);
  if (! moberg_OK(status)) {
    PyErr_Format(PyExc_OSError, "moberg._DigitalIn(%d) failed with %d",
                 index, status.result);
    goto err;
  }
  MobergDigitalInObject *self;
  self = (MobergDigitalInObject *) type->tp_alloc(type, 0);
  if (self == NULL) { goto close; }
  Py_INCREF(moberg_object);
  self->moberg_object = moberg_object;
  self->index = index;
  self->channel = channel;
  return (PyObject *) self;
close:
  status = moberg_digital_in_close(moberg, index, channel);
err:
  return NULL;
}

static int
MobergDigitalIn_init(MobergDigitalInObject *self, PyObject *args, PyObject *kwds)
{
  PyErr_SetString(PyExc_AttributeError, "Not intended to be called");
  return -1;
}

static PyObject *
MobergDigitalIn_read(MobergDigitalInObject *self, PyObject *Py_UNUSED(ignored))
{
  int value;
  struct moberg_status status = self->channel.read(self->channel.context,
                                                   &value);
  if (!moberg_OK(status)) {
    PyErr_Format(PyExc_OSError, "moberg._DigitalIn(%d).read() failed with %d",
                 self->index, status.result);
  }
  return Py_BuildValue("O", value ? Py_True: Py_False);
}

static PyMethodDef MobergDigitalIn_methods[] = {
    {"read", (PyCFunction) MobergDigitalIn_read, METH_NOARGS,
     "Sample and return the DigitalIn value"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject MobergDigitalInType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "moberg.DigitalIn",
    .tp_doc = "DigitalIn objects",
    .tp_basicsize = sizeof(MobergDigitalInObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = MobergDigitalIn_new,
    .tp_init = (initproc) MobergDigitalIn_init,
    .tp_dealloc = (destructor) MobergDigitalIn_dealloc,
    .tp_methods = MobergDigitalIn_methods,
};

/*
 * moberg._DigitalOut class (should never be directly instatiated) 
 */

typedef struct {
  PyObject_HEAD
  PyObject *moberg_object;
  int index;
  struct moberg_digital_out channel;
} MobergDigitalOutObject;

static void
MobergDigitalOut_dealloc(MobergDigitalOutObject *self)
{
  if (self->moberg_object) {
    struct moberg *moberg = ((MobergObject*)self->moberg_object)->moberg;
    struct moberg_status status =
      moberg_digital_out_close(moberg, self->index, self->channel);
    if (! moberg_OK(status)) {
      fprintf(stderr, "Failed to close moberg._DigitalOut(%d) [errno=%d]\n",
              self->index, status.result);
    }
    Py_DECREF(self->moberg_object);
  }
  Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *
MobergDigitalOut_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  PyObject *moberg_object;
  int index;
  struct moberg_digital_out channel = {0};
  
  static char *kwlist[] = { "moberg", "index", NULL };
  if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi", kwlist,
                                    &moberg_object, &index)) {
    goto err;
  }
  if (Py_TYPE(moberg_object) != &MobergType) {
    PyErr_SetString(PyExc_AttributeError, "moberg argument is not Moberg");
    goto err;
  }
  struct moberg *moberg = ((MobergObject*)moberg_object)->moberg;
  struct moberg_status status = moberg_digital_out_open(moberg, index, &channel);
  if (! moberg_OK(status)) {
    PyErr_Format(PyExc_OSError, "moberg._DigitalOut(%d) failed with %d",
                 index, status.result);
    goto err;
  }
  MobergDigitalOutObject *self;
  self = (MobergDigitalOutObject *) type->tp_alloc(type, 0);
  if (self == NULL) { goto close; }
  Py_INCREF(moberg_object);
  self->moberg_object = moberg_object;
  self->index = index;
  self->channel = channel;
  return (PyObject *) self;
close:
  status = moberg_digital_out_close(moberg, index, channel);
err:
  return NULL;
}

static int
MobergDigitalOut_init(MobergDigitalOutObject *self, PyObject *args, PyObject *kwds)
{
  PyErr_SetString(PyExc_AttributeError, "Not intended to be called");
  return -1;
}

static PyObject *
MobergDigitalOut_write(MobergDigitalOutObject *self, PyObject *args)
{
  PyObject *desired;
  int actual;
  if (! PyArg_ParseTuple(args, "O", &desired)) {
    goto err;
  }
  struct moberg_status status = self->channel.write(self->channel.context,
                                                    PyObject_IsTrue(desired),
                                                    &actual);
  if (!moberg_OK(status)) {
    PyErr_Format(PyExc_OSError, "moberg._DigitalOut(%d).write() failed with %d",
                 self->index, status.result);
    goto err;
  }
  return Py_BuildValue("O", actual ? Py_True: Py_False);
err:
  return NULL;
}

static PyMethodDef MobergDigitalOut_methods[] = {
    {"write", (PyCFunction) MobergDigitalOut_write, METH_VARARGS,
     "Set DigitalOut value"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject MobergDigitalOutType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "moberg.DigitalOut",
    .tp_doc = "DigitalOut objects",
    .tp_basicsize = sizeof(MobergDigitalOutObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = MobergDigitalOut_new,
    .tp_init = (initproc) MobergDigitalOut_init,
    .tp_dealloc = (destructor) MobergDigitalOut_dealloc,
    .tp_methods = MobergDigitalOut_methods,
};

/*
 * moberg._EncoderIn class (should never be directly instatiated) 
 */

typedef struct {
  PyObject_HEAD
  PyObject *moberg_object;
  int index;
  struct moberg_encoder_in channel;
} MobergEncoderInObject;

static void
MobergEncoderIn_dealloc(MobergEncoderInObject *self)
{
  if (self->moberg_object) {
    struct moberg *moberg = ((MobergObject*)self->moberg_object)->moberg;
    struct moberg_status status =
      moberg_encoder_in_close(moberg, self->index, self->channel);
    if (! moberg_OK(status)) {
      fprintf(stderr, "Failed to close moberg._EncoderIn(%d) [errno=%d]\n",
              self->index, status.result);
    }
    Py_DECREF(self->moberg_object);
  }
  Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *
MobergEncoderIn_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  PyObject *moberg_object;
  int index;
  struct moberg_encoder_in channel = {0};
  
  static char *kwlist[] = { "moberg", "index", NULL };
  if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi", kwlist,
                                    &moberg_object, &index)) {
    goto err;
  }
  if (Py_TYPE(moberg_object) != &MobergType) {
    PyErr_SetString(PyExc_AttributeError, "moberg argument is not Moberg");
    goto err;
  }
  struct moberg *moberg = ((MobergObject*)moberg_object)->moberg;
  struct moberg_status status = moberg_encoder_in_open(moberg, index, &channel);
  if (! moberg_OK(status)) {
    PyErr_Format(PyExc_OSError, "moberg._EncoderIn(%d) failed with %d",
                 index, status.result);
    goto err;
  }
  MobergEncoderInObject *self;
  self = (MobergEncoderInObject *) type->tp_alloc(type, 0);
  if (self == NULL) { goto close; }
  Py_INCREF(moberg_object);
  self->moberg_object = moberg_object;
  self->index = index;
  self->channel = channel;
  return (PyObject *) self;
close:
  status = moberg_encoder_in_close(moberg, index, channel);
err:
  return NULL;
}

static int
MobergEncoderIn_init(MobergEncoderInObject *self, PyObject *args, PyObject *kwds)
{
  PyErr_SetString(PyExc_AttributeError, "Not intended to be called");
  return -1;
}

static PyObject *
MobergEncoderIn_read(MobergEncoderInObject *self, PyObject *Py_UNUSED(ignored))
{
  long value;
  struct moberg_status status = self->channel.read(self->channel.context,
                                                   &value);
  if (!moberg_OK(status)) {
    PyErr_Format(PyExc_OSError, "moberg._EncoderIn(%d).read() failed with %d",
                 self->index, status.result);
  }
  return Py_BuildValue("l", value);
}

static PyMethodDef MobergEncoderIn_methods[] = {
    {"read", (PyCFunction) MobergEncoderIn_read, METH_NOARGS,
     "Sample and return the EncoderIn value"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject MobergEncoderInType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "moberg.EncoderIn",
    .tp_doc = "EncoderIn objects",
    .tp_basicsize = sizeof(MobergEncoderInObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = MobergEncoderIn_new,
    .tp_init = (initproc) MobergEncoderIn_init,
    .tp_dealloc = (destructor) MobergEncoderIn_dealloc,
    .tp_methods = MobergEncoderIn_methods,
};

/*
 * Module initialization
 */

static PyModuleDef mobergmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "moberg",
    .m_doc = "Moberg I/O interface module.",
    .m_size = -1,
};

#define INITERROR return NULL

PyMODINIT_FUNC
PyInit_moberg(void)
{
  PyObject *m;
  if (PyType_Ready(&MobergType) < 0 ||
      PyType_Ready(&MobergAnalogInType) < 0 ||
      PyType_Ready(&MobergAnalogOutType) < 0 ||
      PyType_Ready(&MobergDigitalInType) < 0 ||
      PyType_Ready(&MobergDigitalOutType) < 0 ||
      PyType_Ready(&MobergEncoderInType) < 0) {
    INITERROR;
  }

  m = PyModule_Create(&mobergmodule);
  if (m == NULL) {
    INITERROR;
  }

  Py_INCREF(&MobergType);
  PyModule_AddObject(m, "Moberg", (PyObject *) &MobergType);
  Py_INCREF(&MobergAnalogInType);
  PyModule_AddObject(m, "_AnalogIn", (PyObject *) &MobergAnalogInType);
  Py_INCREF(&MobergAnalogOutType);
  PyModule_AddObject(m, "_AnalogOut", (PyObject *) &MobergAnalogOutType);
  Py_INCREF(&MobergDigitalInType);
  PyModule_AddObject(m, "_DigitalIn", (PyObject *) &MobergDigitalInType);
  Py_INCREF(&MobergDigitalOutType);
  PyModule_AddObject(m, "_DigitalOut", (PyObject *) &MobergDigitalOutType);
  Py_INCREF(&MobergEncoderInType);
  PyModule_AddObject(m, "_EncoderIn", (PyObject *) &MobergEncoderInType);

  return m;
}
