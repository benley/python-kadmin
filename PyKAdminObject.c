
#include "PyKAdminObject.h"
#include "PyKAdminErrors.h"
#include "PyKAdminIterator.h"
#include "PyKAdminPrincipalObject.h"
#include "PyKAdminPolicyObject.h"

#define IS_NULL(ptr) (ptr == NULL)

static void PyKAdminObject_dealloc(PyKAdminObject *self) {
    
    kadm5_ret_t retval;

    if (!IS_NULL(self->server_handle)) {
        retval = kadm5_destroy(self->server_handle);
        if (retval) {}
    }
    
    if (!IS_NULL(self->context)) {
        krb5_free_context(self->context);
    }

    if (!IS_NULL(self->realm)) {
        free(self->realm);
    }

    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *PyKAdminObject_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {

    PyKAdminObject *self;
    kadm5_ret_t retval;

    self = (PyKAdminObject *)type->tp_alloc(type, 0);

    if (!IS_NULL(self)) {
        if ( (retval = krb5_init_context(&(self->context))) ) {
            Py_DECREF(self);
            return NULL;
        }
    }

    return (PyObject *)self;    

}

static int PyKAdminObject_init(PyKAdminObject *self, PyObject *args, PyObject *kwds) {
    return 0;
}

static PyObject *PyKAdminObject_delete_principal(PyKAdminObject *self, PyObject *args, PyObject *kwds) {

    
    kadm5_ret_t retval;
    krb5_error_code errno;
    krb5_principal princ = NULL;

    char *client_name = NULL;

    if (!PyArg_ParseTuple(args, "s", &client_name))
        return NULL;

    if (!IS_NULL(self->server_handle)) {

        retval = krb5_parse_name(self->context, client_name, &princ);
        if (retval != 0x0) { PyKAdmin_RaiseKAdminError(retval, "krb5_parse_name"); return NULL; }

        retval = kadm5_delete_principal(self->server_handle, princ);
        if (retval != 0x0) { PyKAdmin_RaiseKAdminError(retval, "kadm5_delete_principal"); return NULL; }

    }
    
    krb5_free_principal(self->context, princ);

    Py_RETURN_TRUE;

}


static PyObject *PyKAdminObject_create_principal(PyKAdminObject *self, PyObject *args, PyObject *kwds) {

    //PyObject *result = NULL;

    kadm5_ret_t retval;
    krb5_error_code errno;

    char *princ_name = NULL;
    char *princ_pass = NULL;

    kadm5_principal_ent_rec entry;
    
    memset(&entry, 0, sizeof(entry));
    entry.attributes = 0;
    
    if (!PyArg_ParseTuple(args, "s|z", &princ_name, &princ_pass))
        return NULL;

    if (self->server_handle) {

        if ( (errno = krb5_parse_name(self->context, princ_name, &entry.principal) ) ) {

            printf("Error: krb5_unparse_name [%d]\n", errno);
            return NULL; 
        
        } else {

            retval = kadm5_create_principal(self->server_handle, &entry, KADM5_PRINCIPAL, princ_pass); 
            
            if (retval != 0x0) { PyKAdmin_RaiseKAdminError(retval, "kadm5_create_principal"); return NULL; }
        }
    }

    kadm5_free_principal_ent(self->server_handle, &entry);

    Py_RETURN_TRUE;
}


static PyKAdminPrincipalObject *PyKAdminObject_get_principal(PyKAdminObject *self, PyObject *args, PyObject *kwds) {

    PyKAdminPrincipalObject *principal = NULL;
    char *client_name; 

    if (!PyArg_ParseTuple(args, "s", &client_name))
        return NULL;

    if (!IS_NULL(self->server_handle)) {
        principal = PyKAdminPrincipalObject_create(self, client_name);
    } 

    return principal;
}


static PyKAdminIterator *PyKAdminObject_principal_iter(PyKAdminObject *self, PyObject *args, PyObject *kwds) {

    char *match = NULL;
    PyObject *unpack = Py_False; 
    PyKadminIteratorModes mode = iterate_principals;

    static char *kwlist[] = {"match", "unpack", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|zO", kwlist, &match, &unpack))
        return NULL;

    if (PyObject_IsTrue(unpack))
        mode |= iterate_unpack;

    return PyKAdminIterator_create(self, mode, match);
}

static PyKAdminIterator *PyKAdminObject_policy_iter(PyKAdminObject *self, PyObject *args, PyObject *kwds) {

    char *match = NULL;
    PyObject *unpack = Py_False; 
    PyKadminIteratorModes mode = iterate_policies;

    static char *kwlist[] = {"match", "unpack", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|zO", kwlist, &match, &unpack))
        return NULL;

    if (PyObject_IsTrue(unpack))
        mode |= iterate_unpack;

    return PyKAdminIterator_create(self, mode, match);
}



static PyKAdminPrincipalObject *PyKAdminObject_list_principals(PyKAdminObject *self, PyObject *args, PyObject *kwds) {
    return NULL;
}


static PyMethodDef PyKAdminObject_methods[] = {
    {"getprinc",            (PyCFunction)PyKAdminObject_get_principal,    METH_VARARGS, ""},
    {"get_principal",       (PyCFunction)PyKAdminObject_get_principal,    METH_VARARGS, ""},

    {"delprinc",            (PyCFunction)PyKAdminObject_delete_principal, METH_VARARGS, ""},
    {"delete_principal",    (PyCFunction)PyKAdminObject_delete_principal, METH_VARARGS, ""},

    {"ank",                 (PyCFunction)PyKAdminObject_create_principal, METH_VARARGS, ""},
    {"create_princ",        (PyCFunction)PyKAdminObject_create_principal, METH_VARARGS, ""},
    {"create_principal",    (PyCFunction)PyKAdminObject_create_principal, METH_VARARGS, ""},
    {"list_principals",     (PyCFunction)PyKAdminObject_list_principals,  METH_VARARGS, ""},
    {"principals",          (PyCFunction)PyKAdminObject_principal_iter,   (METH_VARARGS | METH_KEYWORDS), ""},
    {"policies",            (PyCFunction)PyKAdminObject_policy_iter,      (METH_VARARGS | METH_KEYWORDS), ""},
    {NULL, NULL, 0, NULL}
};


PyTypeObject PyKAdminObject_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "kadmin.KAdmin",             /*tp_name*/
    sizeof(PyKAdminObject),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)PyKAdminObject_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "KAdmin objects",           /* tp_doc */
    0,                     /* tp_traverse */
    0,                     /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
    PyKAdminObject_methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)PyKAdminObject_init,      /* tp_init */
    0,                         /* tp_alloc */
    PyKAdminObject_new,                 /* tp_new */
};



PyKAdminObject *PyKAdminObject_create(void) {
    return (PyKAdminObject *)PyKAdminObject_new(&PyKAdminObject_Type, NULL, NULL);
}

void PyKAdminObject_destroy(PyKAdminObject *self) {
    PyKAdminObject_dealloc(self); 
}

