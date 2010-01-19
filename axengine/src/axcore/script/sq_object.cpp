#include "script_p.h"

AX_BEGIN_NAMESPACE

int ScriptMetacall(void *vm, Member *member)
{
	HSQUIRRELVM v = static_cast<HSQUIRRELVM>(vm);
	_CHECK_SELF(ObjectStar, Object_c);

	return 0;
}

#define STACK_RET(name, id)

_IMPL_NATIVE_CONSTRUCTION(Object_c, ObjectStar);

_MEMBER_FUNCTION_IMPL(Object_c, constructor)
{
	StackHandler sa(v);
	_CHECK_SELF(ObjectStar, Object_c);
	self = 0;
	return 1;
}

_MEMBER_FUNCTION_IMPL(Object_c, _get)
{
	StackHandler sa(v);
	_CHECK_SELF(ObjectStar, Object_c);
	ObjectStar obj = *self;
	const SQChar *s = sa.getString(2);
	MetaInfo *metaInfo = obj->getMetaInfo();
	Member *member = metaInfo->findMember(s);

	if (!member) return SQ_ERROR;

	if (member->isProperty()) {
		Variant::TypeId propType = member->getPropType();
		Variant arg(propType);

		bool success = member->getProperty(obj, arg.getPointer());
		if (!success)
			return SQ_ERROR;

		return sa.retRawData(Argument(arg.getTypeId(), arg.getPointer()));
	}

	return SQ_ERROR;
//	return sa.Return(member->m_scriptClosure);
}

_MEMBER_FUNCTION_IMPL(Object_c, _set)
{
	StackHandler sa(v);
	_CHECK_SELF(ObjectStar, Object_c);
	ObjectStar obj = *self;
	const SQChar *s = sa.getString(2);
	MetaInfo *metaInfo = obj->getMetaInfo();
	Member *member = metaInfo->findMember(s);

	if (!member || !member->isProperty()) return SQ_ERROR;

	Variant::TypeId propType = member->getPropType();
	Result value;
	sa.getRawData(3, value);

	// type is matched
	if (value.typeId == propType) {
		bool success = member->setProperty(obj, value.data);
		return success ? 0 : SQ_ERROR;
	}

	Variant castTo(propType);
	bool success = Variant::rawCast(value.typeId, value.data, propType, castTo.getPointer());
	if (!success) {
		return SQ_ERROR;
	}

	success = member->setProperty(obj, castTo.getPointer());

	return success ? 0 : SQ_ERROR;
}

_BEGIN_CLASS(Object_c)
_MEMBER_FUNCTION(Object_c,constructor,-1,_SC("."))
_MEMBER_FUNCTION(Object_c,_set,3,_SC("xs|n"))
_MEMBER_FUNCTION(Object_c,_get,2,_SC("xs|n"))
_END_CLASS(Object_c)

AX_END_NAMESPACE