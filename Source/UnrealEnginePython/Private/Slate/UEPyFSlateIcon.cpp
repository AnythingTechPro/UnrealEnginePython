
#include "UEPyFSlateIcon.h"
#include "Engine/Texture2D.h"
#include "Engine/Engine.h"

static PyObject *py_ue_fslate_icon_get_icon(ue_PyFSlateIcon *self, PyObject * args)
{
	return py_ue_new_uscriptstruct(FSlateBrush::StaticStruct(), (uint8*)self->icon.GetIcon());
}

static PyMethodDef ue_PyFSlateIcon_methods[] = {
	{ "get_icon", (PyCFunction)py_ue_fslate_icon_get_icon, METH_VARARGS, "" },
	{ NULL }  /* Sentinel */
};

static PyObject *ue_PyFSlateIcon_str(ue_PyFSlateIcon *self)
{
	return PyUnicode_FromFormat("<unreal_engine.SlateIcon {'name': %s}>",
		TCHAR_TO_UTF8(*self->icon.GetStyleName().ToString()));
}

static PyTypeObject ue_PyFSlateIconType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"unreal_engine.FSlateIcon", /* tp_name */
	sizeof(ue_PyFSlateIcon), /* tp_basicsize */
	0,                         /* tp_itemsize */
	0,       /* tp_dealloc */
	0,                         /* tp_print */
	0,                         /* tp_getattr */
	0,                         /* tp_setattr */
	0,                         /* tp_reserved */
	0,                         /* tp_repr */
	0,                         /* tp_as_number */
	0,                         /* tp_as_sequence */
	0,                         /* tp_as_mapping */
	0,                         /* tp_hash  */
	0,                         /* tp_call */
	(reprfunc)ue_PyFSlateIcon_str,                         /* tp_str */
	0,                         /* tp_getattro */
	0,                         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"Unreal Engine FSlateIcon",           /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	ue_PyFSlateIcon_methods,             /* tp_methods */
};

static int ue_py_fslate_icon_init(ue_PyFSlateIcon *self, PyObject *args, PyObject *kwargs)
{
	char *style_set = nullptr;
	char *style = nullptr;
	if (!PyArg_ParseTuple(args, "|ss", &style_set, &style))
	{
		return -1;
	}

	if (style_set)
	{
		if (!style)
		{
			PyErr_SetString(PyExc_ValueError, "you have not specified as style name");
			return -1;
		}

		ISlateStyle const* const foundStyleSet = FSlateStyleRegistry::FindSlateStyle(FName(style_set));

		const FSlateBrush * iconBrush = foundStyleSet->GetBrush(style);
		FString Path = iconBrush->GetResourceName().ToString();

		// Hack to load in the texture resource object of the icon brush in case it hasn't been already loaded
		if (!Path.IsEmpty() && iconBrush->GetResourceObject() == nullptr)
		{
            if (Path.StartsWith(FSlateBrush::UTextureIdentifier()))
            {
				Path = Path.RightChop(FSlateBrush::UTextureIdentifier().Len());
            }

			UObject* TextureObject = LoadObject<UTexture2D>(NULL, *Path, NULL, LOAD_None, NULL);
			FSlateBrush* Brush = const_cast<FSlateBrush*>(iconBrush);

			// Set the texture object to a default texture to prevent constant loading of missing textures
			if (!TextureObject)
			{
				UE_LOG(LogSlate, Warning, TEXT("Error loading loading UTexture from path: %s not found"), *Path);
                if (GEngine)
                {
					TextureObject = GEngine->DefaultTexture;
                }
			}
			else
			{
				// We do this here because this deprecated system of loading textures will not report references and we dont want the Slate RHI resource manager to manage references
				TextureObject->AddToRoot();
			}

            if (TextureObject)
            {
				Brush->SetResourceObject(TextureObject);
            }
		}

		new(&self->icon) FSlateIcon(FName(style_set), FName(style));
	}
	else
	{
		new(&self->icon) FSlateIcon();
	}
	return 0;
}

void ue_python_init_fslate_icon(PyObject *ue_module)
{
	ue_PyFSlateIconType.tp_new = PyType_GenericNew;

	ue_PyFSlateIconType.tp_init = (initproc)ue_py_fslate_icon_init;

	if (PyType_Ready(&ue_PyFSlateIconType) < 0)
		return;

	Py_INCREF(&ue_PyFSlateIconType);
	PyModule_AddObject(ue_module, "FSlateIcon", (PyObject *)&ue_PyFSlateIconType);
}

ue_PyFSlateIcon *py_ue_new_fslate_icon(const FSlateIcon slate_icon)
{
	ue_PyFSlateIcon *ret = (ue_PyFSlateIcon *)PyObject_New(ue_PyFSlateIcon, &ue_PyFSlateIconType);
	ret->icon = slate_icon;
	return ret;
}

ue_PyFSlateIcon *py_ue_is_fslate_icon(PyObject *obj)
{
	if (!PyObject_IsInstance(obj, (PyObject *)&ue_PyFSlateIconType))
		return nullptr;
	return (ue_PyFSlateIcon *)obj;
}
