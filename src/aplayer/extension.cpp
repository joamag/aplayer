/*
 Hive Audio Player
 Copyright (c) 2008-2017 Hive Solutions Lda.

 This file is part of Hive Audio Player.

 Hive Audio Player is free software: you can redistribute it and/or modify
 it under the terms of the Apache License as published by the Apache
 Foundation, either version 2.0 of the License, or (at your option) any
 later version.

 Hive Audio Player is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 Apache License for more details.

 You should have received a copy of the Apache License along with
 Hive Audio Player. If not, see <http://www.apache.org/licenses/>.

 __author__    = João Magalhães <joamag@hive.pt>
 __version__   = 1.0.0
 __revision__  = $LastChangedRevision$
 __date__      = $LastChangedDate$
 __copyright__ = Copyright (c) 2008-2017 Hive Solutions Lda.
 __license__   = Apache License, Version 2.0
*/

#include "stdafx.h"

#include "extension.h"

PyMethodDef aplayer_functions[4] = {
    {
        "play",
        extension_play,
        METH_VARARGS,
        NULL
    },
    {
        "register",
        extension_play,
        METH_NOARGS,
        NULL
    },
    {
        "unregister",
        extension_play,
        METH_NOARGS,
        NULL
    },
    {
        NULL,
        NULL,
        0,
        NULL
    }
};

PyObject *extension_register(PyObject *self, PyObject *args) {
    // registers the aplayer structures, starting both
    // the hardware and logical "items"
    register_aplayer();

    Py_RETURN_NONE;
};

PyObject *extension_unregister(PyObject *self, PyObject *args) {
    // unregisters th aplayer structures, stopping both
    // the hardware and logical "items"
    unregister_aplayer();

    Py_RETURN_NONE;
};

PyObject *extension_play(PyObject *self, PyObject *args) {
    char *filename;

    if(PyArg_ParseTuple(args, "s", &filename) == 0) { return NULL; }

    Py_BEGIN_ALLOW_THREADS
    struct aplayer_t player;
    open_aplayer(filename, &player);
    play_aplayer(&player);
    close_aplayer(&player);
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
};

PyMODINIT_FUNC initaplayer(void) {
    // allocates space for the module object to hold the
    // module to be created
    PyObject *aplayer_module;

    // creates the aplayer extension module with the
    // functions defined in the previous array
    aplayer_module = Py_InitModule("aplayer", aplayer_functions);
    if(aplayer_module == NULL) { return; }

    // runs the registeration of the aplayer structures
    // this could be run manually using the register function
    register_aplayer();
}
