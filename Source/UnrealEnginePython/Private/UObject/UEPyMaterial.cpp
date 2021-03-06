
#include "UEPyMaterial.h"

#if WITH_EDITOR
#include "Editor/UnrealEd/Classes/MaterialGraph/MaterialGraph.h"
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
#include "Editor/UnrealEd/Classes/MaterialGraph/MaterialGraphSchema.h"
#endif
#include <Materials/MaterialInterface.h>
#include <Shader.h>
#include <MaterialShared.h>
#include "Materials/Material.h"

PyObject *py_ue_get_material_instruction_count(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);
	uint64 feature_level = ERHIFeatureLevel::SM5, material_quality = EMaterialQualityLevel::High, shader_platform = EShaderPlatform::SP_PCD3D_SM5;
	if (!PyArg_ParseTuple(args, "KKK:get_material_instruction_count", &feature_level, &material_quality, &shader_platform))
	{
		return nullptr;
	}

	if (!self->ue_object->GetClass()->IsChildOf<UMaterialInterface>())
	{
		return PyErr_Format(PyExc_Exception, "uobject is not a UMaterialInterface");
	}

	UMaterialInterface *material = (UMaterialInterface *)self->ue_object;
	UMaterialInterface *old_material =	nullptr;
	FMaterialResource * material_resource = material->GetMaterialResource((ERHIFeatureLevel::Type)feature_level, (EMaterialQualityLevel::Type)material_quality);
	int instruction_count = 0;
	if (material_resource == nullptr)
	{
		// If this is an instance, will get the resource from parent material
		material = material->GetMaterial();
	}

	material_resource = material->GetMaterialResource((ERHIFeatureLevel::Type)feature_level, (EMaterialQualityLevel::Type)material_quality);
	FMaterialShaderMapId OutId;
	TArray<FString> Descriptions;
	TArray<int32> InstructionCounts;
	
	material_resource->GetRepresentativeInstructionCounts(Descriptions, InstructionCounts);

	// [0] = Base pass shader count, [Last] = Vertex shader, discarding the other two as they are permutations of 0 for surface/volumetric lightmap cases
	instruction_count = InstructionCounts.Last() + InstructionCounts[0];

	return PyLong_FromLong(instruction_count);
}

PyObject *py_ue_get_material_sampler_count(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);
	uint64 feature_level = ERHIFeatureLevel::SM5, material_quality = EMaterialQualityLevel::High, shader_platform = EShaderPlatform::SP_PCD3D_SM5;
	if (!PyArg_ParseTuple(args, "KKK:get_material_sampler_count", &feature_level, &material_quality, &shader_platform))
	{
		return nullptr;
	}

	if (!self->ue_object->GetClass()->IsChildOf<UMaterialInterface>())
	{
		return PyErr_Format(PyExc_Exception, "uobject is not a UMaterialInterface");
	}

	UMaterialInterface *material = (UMaterialInterface *)self->ue_object;
	UMaterialInterface *old_material = nullptr;
	FMaterialResource * material_resource = material->GetMaterialResource((ERHIFeatureLevel::Type)feature_level, (EMaterialQualityLevel::Type)material_quality);
	if (material_resource == nullptr)
	{
		// If this is an instance, will get the resource from parent material
		material = material->GetMaterial();
	}

	material_resource = material->GetMaterialResource((ERHIFeatureLevel::Type)feature_level, (EMaterialQualityLevel::Type)material_quality);
	FMaterialShaderMapId OutId;
	material_resource->GetShaderMapId((EShaderPlatform)shader_platform, OutId);

	int sampler_count = 0;
	sampler_count += material_resource->GetSamplerUsage();

	return PyLong_FromLong(sampler_count);
}

#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Wrappers/UEPyFLinearColor.h"
#include "Wrappers/UEPyFVector.h"
#include "Engine/Texture.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/StaticMesh.h"

PyObject *py_ue_set_material_by_name(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	char *slot_name;
	PyObject *py_mat;
	if (!PyArg_ParseTuple(args, "sO:set_material_by_name", &slot_name, &py_mat))
	{
		return nullptr;
	}

	UPrimitiveComponent *primitive = ue_py_check_type<UPrimitiveComponent>(self);
	if (!primitive)
		return PyErr_Format(PyExc_Exception, "uobject is not a UPrimitiveComponent");

	UMaterialInterface *material = ue_py_check_type<UMaterialInterface>(py_mat);
	if (!material)
		return PyErr_Format(PyExc_Exception, "argument is not a UMaterialInterface");

	primitive->SetMaterialByName(FName(UTF8_TO_TCHAR(slot_name)), material);

	Py_RETURN_NONE;
}

PyObject *py_ue_set_material(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	int slot;
	PyObject *py_mat;
	if (!PyArg_ParseTuple(args, "iO:set_material", &slot, &py_mat))
	{
		return nullptr;
	}

	UPrimitiveComponent *primitive = ue_py_check_type<UPrimitiveComponent>(self);
	if (!primitive)
		return PyErr_Format(PyExc_Exception, "uobject is not a UPrimitiveComponent");

	UMaterialInterface *material = ue_py_check_type<UMaterialInterface>(py_mat);
	if (!material)
		return PyErr_Format(PyExc_Exception, "argument is not a UMaterialInterface");

	primitive->SetMaterial(slot, material);

	Py_RETURN_NONE;
}

PyObject *py_ue_set_material_scalar_parameter(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	char *scalarName = nullptr;
	float scalarValue = 0;
	if (!PyArg_ParseTuple(args, "sf:set_material_scalar_parameter", &scalarName, &scalarValue))
	{
		return NULL;
	}

	FName parameterName(UTF8_TO_TCHAR(scalarName));

	bool valid = false;

#if WITH_EDITOR
	if (self->ue_object->IsA<UMaterialInstanceConstant>())
	{
		UMaterialInstanceConstant *material_instance = (UMaterialInstanceConstant *)self->ue_object;
		material_instance->SetScalarParameterValueEditorOnly(parameterName, scalarValue);
		valid = true;
	}
#endif

	if (self->ue_object->IsA<UMaterialInstanceDynamic>())
	{
		UMaterialInstanceDynamic *material_instance = (UMaterialInstanceDynamic *)self->ue_object;
		material_instance->SetScalarParameterValue(parameterName, scalarValue);
		valid = true;
	}

	if (!valid)
	{
		return PyErr_Format(PyExc_Exception, "uobject is not a MaterialInstance");
	}

	Py_RETURN_NONE;

}

PyObject *py_ue_get_material_scalar_parameter(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	char *scalarName = nullptr;
	if (!PyArg_ParseTuple(args, "s:get_material_scalar_parameter", &scalarName))
	{
		return NULL;
	}

	if (!self->ue_object->IsA<UMaterialInstance>())
	{
		return PyErr_Format(PyExc_Exception, "uobject is not a UMaterialInstance");
	}

	FName parameterName(UTF8_TO_TCHAR(scalarName));

	UMaterialInstance *material_instance = (UMaterialInstance *)self->ue_object;

	float value = 0;

	material_instance->GetScalarParameterValue(parameterName, value);

	return PyFloat_FromDouble(value);

}

PyObject *py_ue_get_material_static_switch_parameter(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	char *switchName = nullptr;
	if (!PyArg_ParseTuple(args, "s:get_material_static_switch_parameter", &switchName))
	{
		return NULL;
	}

	if (!self->ue_object->IsA<UMaterialInstance>())
	{
		return PyErr_Format(PyExc_Exception, "uobject is not a UMaterialInstance");
	}

	FName parameterName(UTF8_TO_TCHAR(switchName));

	UMaterialInstance *material_instance = (UMaterialInstance *)self->ue_object;

	bool value = false;

	FGuid guid;
	material_instance->GetStaticSwitchParameterValue(parameterName, value, guid);

	if (value)
	{
		Py_RETURN_TRUE;
	}

	Py_RETURN_FALSE;
}

PyObject *py_ue_set_material_vector_parameter(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);

	char *vectorName;
	PyObject *vectorValue = nullptr;
	if (!PyArg_ParseTuple(args, "sO:set_material_vector_parameter", &vectorName, &vectorValue))
	{
		return NULL;
	}

	FLinearColor vectorParameter(0, 0, 0, 0);

	ue_PyFLinearColor *py_vector = py_ue_is_flinearcolor(vectorValue);
	if (!py_vector)
	{
		ue_PyFVector *py_true_vector = py_ue_is_fvector(vectorValue);
		if (!py_true_vector)
		{
			return PyErr_Format(PyExc_Exception, "argument must be an FLinearColor or FVector");
		}
		else
		{
			vectorParameter.R = py_ue_fvector_get(py_true_vector).X;
			vectorParameter.G = py_ue_fvector_get(py_true_vector).Y;
			vectorParameter.B = py_ue_fvector_get(py_true_vector).Z;
			vectorParameter.A = 1;
		}
	}
	else
	{
		vectorParameter = py_vector->color;
	}

	FName parameterName(UTF8_TO_TCHAR(vectorName));

	bool valid = false;

#if WITH_EDITOR
	if (self->ue_object->IsA<UMaterialInstanceConstant>())
	{
		UMaterialInstanceConstant *material_instance = (UMaterialInstanceConstant *)self->ue_object;
		material_instance->SetVectorParameterValueEditorOnly(parameterName, vectorParameter);
		valid = true;
	}
#endif

	if (self->ue_object->IsA<UMaterialInstanceDynamic>())
	{
		UMaterialInstanceDynamic *material_instance = (UMaterialInstanceDynamic *)self->ue_object;
		material_instance->SetVectorParameterValue(parameterName, vectorParameter);
		valid = true;
	}

	if (!valid)
	{
		return PyErr_Format(PyExc_Exception, "uobject is not a MaterialInstance");
	}

	Py_RETURN_NONE;
}

PyObject *py_ue_get_material_vector_parameter(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	char *scalarName = nullptr;
	if (!PyArg_ParseTuple(args, "s:get_material_vector_parameter", &scalarName))
	{
		return NULL;
	}

	if (!self->ue_object->IsA<UMaterialInstance>())
	{
		return PyErr_Format(PyExc_Exception, "uobject is not a UMaterialInstance");
	}

	FName parameterName(UTF8_TO_TCHAR(scalarName));

	UMaterialInstance *material_instance = (UMaterialInstance *)self->ue_object;

	FLinearColor vec(0, 0, 0);

	material_instance->GetVectorParameterValue(parameterName, vec);

	return py_ue_new_flinearcolor(vec);
}


PyObject *py_ue_set_material_texture_parameter(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);

	char *textureName;
	PyObject *textureObject = nullptr;
	if (!PyArg_ParseTuple(args, "sO:set_texture_parameter", &textureName, &textureObject))
	{
		return NULL;
	}

	if (!ue_is_pyuobject(textureObject))
	{
		return PyErr_Format(PyExc_Exception, "argument is not a UObject");
	}

	ue_PyUObject *py_obj = (ue_PyUObject *)textureObject;
	if (!py_obj->ue_object->IsA<UTexture>())
		return PyErr_Format(PyExc_Exception, "uobject is not a UTexture");

	UTexture *ue_texture = (UTexture *)py_obj->ue_object;

	FName parameterName(UTF8_TO_TCHAR(textureName));

	bool valid = false;

#if WITH_EDITOR
	if (self->ue_object->IsA<UMaterialInstanceConstant>())
	{
		UMaterialInstanceConstant *material_instance = (UMaterialInstanceConstant *)self->ue_object;
		material_instance->SetTextureParameterValueEditorOnly(parameterName, ue_texture);
		valid = true;
	}
#endif

	if (self->ue_object->IsA<UMaterialInstanceDynamic>())
	{
		UMaterialInstanceDynamic *material_instance = (UMaterialInstanceDynamic *)self->ue_object;
		material_instance->SetTextureParameterValue(parameterName, ue_texture);
		valid = true;
	}

	if (!valid)
	{
		return PyErr_Format(PyExc_Exception, "uobject is not a MaterialInstance");
	}

	Py_RETURN_NONE;
}

PyObject *py_ue_get_material_texture_parameter(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	char *scalarName = nullptr;
	if (!PyArg_ParseTuple(args, "s:get_material_texture_parameter", &scalarName))
	{
		return NULL;
	}

	if (!self->ue_object->IsA<UMaterialInstance>())
	{
		return PyErr_Format(PyExc_Exception, "uobject is not a UMaterialInstance");
	}

	FName parameterName(UTF8_TO_TCHAR(scalarName));

	UMaterialInstance *material_instance = (UMaterialInstance *)self->ue_object;

	UTexture *texture = nullptr;

	if (!material_instance->GetTextureParameterValue(parameterName, texture))
	{
		Py_RETURN_NONE;
	}

	Py_RETURN_UOBJECT(texture);
}

PyObject *py_ue_create_material_instance_dynamic(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	PyObject *py_material = nullptr;

	if (!PyArg_ParseTuple(args, "O:create_material_instance_dynamic", &py_material))
	{
		return nullptr;
	}

	UMaterialInterface *material_interface = ue_py_check_type<UMaterialInterface>(py_material);
	if (!material_interface)
		return PyErr_Format(PyExc_Exception, "argument is not a UMaterialInterface");

	UMaterialInstanceDynamic *material_dynamic = UMaterialInstanceDynamic::Create(material_interface, self->ue_object);

	Py_RETURN_UOBJECT(material_dynamic);
}



#if WITH_EDITOR
PyObject *py_ue_set_material_parent(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);


	PyObject *py_material = nullptr;

	if (!PyArg_ParseTuple(args, "O:set_parent", &py_material))
	{
		return NULL;
	}

	if (!self->ue_object->IsA<UMaterialInstanceConstant>())
	{
		return PyErr_Format(PyExc_Exception, "uobject is not a UMaterialInstanceConstant");
	}


	if (!ue_is_pyuobject(py_material))
	{
		return PyErr_Format(PyExc_Exception, "argument is not a UObject");
	}

	ue_PyUObject *py_obj = (ue_PyUObject *)py_material;
	if (!py_obj->ue_object->IsA<UMaterialInterface>())
		return PyErr_Format(PyExc_Exception, "uobject is not a UMaterialInterface");


	UMaterialInterface *materialInterface = (UMaterialInterface *)py_obj->ue_object;
	UMaterialInstanceConstant *material_instance = (UMaterialInstanceConstant *)self->ue_object;
	material_instance->SetParentEditorOnly(materialInterface);
	material_instance->PostEditChange();

	Py_RETURN_NONE;

}

PyObject *py_ue_static_mesh_set_collision_for_lod(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);

	int lod_index;
	int material_index;
	PyObject *py_bool;
	if (!PyArg_ParseTuple(args, "iiO:static_mesh_set_collision_for_lod", &lod_index, &material_index, &py_bool))
	{
		return NULL;
	}


	if (!self->ue_object->IsA<UStaticMesh>())
	{
		return PyErr_Format(PyExc_Exception, "uobject is not a StaticMesh");
	}

	UStaticMesh *mesh = (UStaticMesh *)self->ue_object;

	bool enabled = false;
	if (PyObject_IsTrue(py_bool))
	{
		enabled = true;
	}

	FMeshSectionInfo info = mesh->SectionInfoMap.Get(lod_index, material_index);
	info.bEnableCollision = enabled;
	mesh->SectionInfoMap.Set(lod_index, material_index, info);

	mesh->MarkPackageDirty();

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject *py_ue_static_mesh_set_shadow_for_lod(ue_PyUObject *self, PyObject * args)
{
	ue_py_check(self);

	int lod_index;
	int material_index;
	PyObject *py_bool;
	if (!PyArg_ParseTuple(args, "iiO:static_mesh_set_shadow_for_lod", &lod_index, &material_index, &py_bool))
	{
		return NULL;
	}


	if (!self->ue_object->IsA<UStaticMesh>())
	{
		return PyErr_Format(PyExc_Exception, "uobject is not a StaticMesh");
	}

	UStaticMesh *mesh = (UStaticMesh *)self->ue_object;

	bool enabled = false;
	if (PyObject_IsTrue(py_bool))
	{
		enabled = true;
	}

	FMeshSectionInfo info = mesh->SectionInfoMap.Get(lod_index, material_index);
	info.bCastShadow = enabled;
	mesh->SectionInfoMap.Set(lod_index, material_index, info);

	mesh->MarkPackageDirty();

	Py_RETURN_NONE;
}

PyObject *py_ue_get_material_graph(ue_PyUObject *self, PyObject * args)
{

	ue_py_check(self);

	UMaterial *material = ue_py_check_type<UMaterial>(self);
	if (!material)
		return PyErr_Format(PyExc_Exception, "uobject is not a UMaterialInterface");

	UMaterialGraph *graph = material->MaterialGraph;
	if (!graph)
	{
		graph = (UMaterialGraph *)FBlueprintEditorUtils::CreateNewGraph(material, NAME_None, UMaterialGraph::StaticClass(), UMaterialGraphSchema::StaticClass());
		material->MaterialGraph = graph;
	}
	if (!graph)
		return PyErr_Format(PyExc_Exception, "Unable to retrieve/allocate MaterialGraph");

	Py_RETURN_UOBJECT(graph);
}

#endif
