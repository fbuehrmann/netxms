/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: nxsl_classes.cpp
**
**/

#include "nxcore.h"
#include <entity_mib.h>
#include <ethernet_ip.h>

/**
 * Get ICMP statistic for node sub-object
 */
template <class O> static NXSL_Value *GetObjectIcmpStatistic(O *object, IcmpStatFunction function, NXSL_VM *vm)
{
   NXSL_Value *value;
   Node *parentNode = object->getParentNode();
   if (parentNode != NULL)
   {
      TCHAR target[MAX_OBJECT_NAME + 5], buffer[MAX_RESULT_LENGTH];
      _sntprintf(target, MAX_OBJECT_NAME + 4, _T("X(N:%s)"), object->getName());
      if (parentNode->getIcmpStatistic(target, function, buffer) == DCE_SUCCESS)
      {
         value = vm->createValue(buffer);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else
   {
      value = vm->createValue();
   }
   return value;
}

/**
 * NetObj::bind(object)
 */
NXSL_METHOD_DEFINITION(NetObj, bind)
{
   NetObj *thisObject = static_cast<NetObj*>(object->getData());
   if ((thisObject->getObjectClass() != OBJECT_CONTAINER) && (thisObject->getObjectClass() != OBJECT_SERVICEROOT))
      return NXSL_ERR_BAD_CLASS;

   NXSL_Object *nxslChild = argv[0]->getValueAsObject();
   if (!nxslChild->getClass()->instanceOf(g_nxslNetObjClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   NetObj *child = static_cast<NetObj*>(nxslChild->getData());
   if (!IsValidParentClass(child->getObjectClass(), thisObject->getObjectClass()))
      return NXSL_ERR_BAD_CLASS;

   if (child->isChild(thisObject->getId())) // prevent loops
      return NXSL_ERR_INVALID_OBJECT_OPERATION;

   thisObject->addChild(child);
   child->addParent(thisObject);
   thisObject->calculateCompoundStatus();

   *result = vm->createValue();
   return 0;
}

/**
 * NetObj::bindTo(object)
 */
NXSL_METHOD_DEFINITION(NetObj, bindTo)
{
   NetObj *thisObject = static_cast<NetObj*>(object->getData());

   NXSL_Object *nxslParent = argv[0]->getValueAsObject();
   if (!nxslParent->getClass()->instanceOf(g_nxslNetObjClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   NetObj *parent = static_cast<NetObj*>(nxslParent->getData());
   if ((parent->getObjectClass() != OBJECT_CONTAINER) && (parent->getObjectClass() != OBJECT_SERVICEROOT))
      return NXSL_ERR_BAD_CLASS;

   if (!IsValidParentClass(thisObject->getObjectClass(), parent->getObjectClass()))
      return NXSL_ERR_BAD_CLASS;

   if (thisObject->isChild(parent->getId())) // prevent loops
      return NXSL_ERR_INVALID_OBJECT_OPERATION;

   parent->addChild(thisObject);
   thisObject->addParent(parent);
   parent->calculateCompoundStatus();

   *result = vm->createValue();
   return 0;
}

/**
 * NetObj::clearGeoLocation()
 */
NXSL_METHOD_DEFINITION(NetObj, clearGeoLocation)
{
   static_cast<NetObj*>(object->getData())->setGeoLocation(GeoLocation());
   *result = vm->createValue();
   return 0;
}

/**
 * NetObj::delete()
 */
NXSL_METHOD_DEFINITION(NetObj, delete)
{
   static_cast<NetObj*>(object->getData())->deleteObject();
   *result = vm->createValue();
   return 0;
}

/**
 * NetObj::deleteCustomAttribute(name)
 * Returns last attribute value
 */
NXSL_METHOD_DEFINITION(NetObj, deleteCustomAttribute)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   NetObj *netobj = static_cast<NetObj*>(object->getData());
   const TCHAR *name = argv[0]->getValueAsCString();
   NXSL_Value *value = netobj->getCustomAttributeForNXSL(vm, name);
   *result = (value != NULL) ? value : vm->createValue();
   netobj->deleteCustomAttribute(name);
   return 0;
}

/**
 * NetObj::enterMaintenance(comment)
 */
NXSL_METHOD_DEFINITION(NetObj, enterMaintenance)
{
   if (argc > 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if ((argc != 0) && !argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   static_cast<NetObj*>(object->getData())->enterMaintenanceMode((argc != 0) ? argv[0]->getValueAsCString() : NULL);
   *result = vm->createValue();
   return 0;
}

/**
 * NetObj::expandString() method
 */
NXSL_METHOD_DEFINITION(NetObj, expandString)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   NetObj *n = static_cast<NetObj*>(object->getData());
   *result = vm->createValue(n->expandText(argv[0]->getValueAsCString(), NULL, NULL, NULL, NULL, NULL, NULL));
   return 0;
}

/**
 * NetObj::getCustomAttribute(name)
 */
NXSL_METHOD_DEFINITION(NetObj, getCustomAttribute)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   NXSL_Value *value = static_cast<NetObj*>(object->getData())->getCustomAttributeForNXSL(vm, argv[0]->getValueAsCString());
   *result = (value != NULL) ? value : vm->createValue();
   return 0;
}

/**
 * NetObj::leaveMaintenance()
 */
NXSL_METHOD_DEFINITION(NetObj, leaveMaintenance)
{
   static_cast<NetObj*>(object->getData())->leaveMaintenanceMode();
   *result = vm->createValue();
   return 0;
}

/**
 * NetObj::manage()
 */
NXSL_METHOD_DEFINITION(NetObj, manage)
{
   static_cast<NetObj*>(object->getData())->setMgmtStatus(true);
   *result = vm->createValue();
   return 0;
}

/**
 * NetObj::rename(name)
 */
NXSL_METHOD_DEFINITION(NetObj, rename)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   static_cast<NetObj*>(object->getData())->setName(argv[0]->getValueAsCString());
   *result = vm->createValue();
   return 0;
}

/**
 * setComments(text)
 */
NXSL_METHOD_DEFINITION(NetObj, setComments)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   static_cast<NetObj*>(object->getData())->setComments(MemCopyString(argv[0]->getValueAsCString()));
   *result = vm->createValue();
   return 0;
}

/**
 * NetObj::setCustomAttribute(name, value, ...)
 */
NXSL_METHOD_DEFINITION(NetObj, setCustomAttribute)
{
   if (argc > 3 || argc < 2)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString() || !argv[1]->isString())
      return NXSL_ERR_NOT_STRING;

   if(argc == 3 && !argv[2]->isBoolean())
      return NXSL_ERR_NOT_BOOLEAN;

   NetObj *netxmsObject = static_cast<NetObj*>(object->getData());
   const TCHAR *name = argv[0]->getValueAsCString();
   NXSL_Value *value = netxmsObject->getCustomAttributeForNXSL(vm, name);
   *result = (value != NULL) ? value : vm->createValue(); // Return NULL if attribute not found
   StateChange inherit = (argc == 3) ? (argv[2]->getValueAsBoolean() ? StateChange::SET : StateChange::CLEAR) : StateChange::IGNORE;
   netxmsObject->setCustomAttribute(name, argv[1]->getValueAsCString(), inherit);
   return 0;
}

/**
 * setGeoLocation(loc)
 */
NXSL_METHOD_DEFINITION(NetObj, setGeoLocation)
{
   if (!argv[0]->isObject())
      return NXSL_ERR_NOT_OBJECT;

   NXSL_Object *o = argv[0]->getValueAsObject();
   if (_tcscmp(o->getClass()->getName(), g_nxslGeoLocationClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   GeoLocation *gl = (GeoLocation *)o->getData();
   static_cast<NetObj*>(object->getData())->setGeoLocation(*gl);
   *result = vm->createValue();
   return 0;
}

/**
 * setMapImage(image)
 */
NXSL_METHOD_DEFINITION(NetObj, setMapImage)
{
   if (!argv[0]->isString() && !argv[0]->isNull())
      return NXSL_ERR_NOT_STRING;

   bool success = false;
   if (argv[0]->isNull() || !_tcscmp(argv[0]->getValueAsCString(), _T("00000000-0000-0000-0000-000000000000")))
   {
      static_cast<NetObj*>(object->getData())->setMapImage(uuid::NULL_UUID);
      success = true;
   }
   else
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT guid FROM images WHERE upper(guid)=upper(?) OR upper(name)=upper(?)"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, argv[0]->getValueAsCString(), DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, argv[0]->getValueAsCString(), DB_BIND_STATIC);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != NULL)
         {
            if (DBGetNumRows(hResult) > 0)
            {
               uuid guid = DBGetFieldGUID(hResult, 0, 0);
               static_cast<NetObj*>(object->getData())->setMapImage(guid);
               success = true;
            }
            DBFreeResult(hResult);
         }
         DBFreeStatement(hStmt);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }

   *result = vm->createValue(success ? 1 : 0);
   return 0;
}

/**
 * NetObj::setStatusCalculation(type, ...)
 */
NXSL_METHOD_DEFINITION(NetObj, setStatusCalculation)
{
   if (argc < 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   INT32 success = 1;
   int method = argv[0]->getValueAsInt32();
   NetObj *netobj = (NetObj *)object->getData();
   switch(method)
   {
      case SA_CALCULATE_DEFAULT:
      case SA_CALCULATE_MOST_CRITICAL:
         netobj->setStatusCalculation(method);
         break;
      case SA_CALCULATE_SINGLE_THRESHOLD:
         if (argc < 2)
            return NXSL_ERR_INVALID_ARGUMENT_COUNT;
         if (!argv[1]->isInteger())
            return NXSL_ERR_NOT_INTEGER;
         netobj->setStatusCalculation(method, argv[1]->getValueAsInt32());
         break;
      case SA_CALCULATE_MULTIPLE_THRESHOLDS:
         if (argc < 5)
            return NXSL_ERR_INVALID_ARGUMENT_COUNT;
         for(int i = 1; i <= 4; i++)
         {
            if (!argv[i]->isInteger())
               return NXSL_ERR_NOT_INTEGER;
         }
         netobj->setStatusCalculation(method, argv[1]->getValueAsInt32(), argv[2]->getValueAsInt32(), argv[3]->getValueAsInt32(), argv[4]->getValueAsInt32());
         break;
      default:
         success = 0;   // invalid method
         break;
   }
   *result = vm->createValue(success);
   return 0;
}

/**
 * NetObj::setStatusPropagation(type, ...)
 */
NXSL_METHOD_DEFINITION(NetObj, setStatusPropagation)
{
   if (argc < 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   INT32 success = 1;
   int method = argv[0]->getValueAsInt32();
   NetObj *netobj = (NetObj *)object->getData();
   switch(method)
   {
      case SA_PROPAGATE_DEFAULT:
      case SA_PROPAGATE_UNCHANGED:
         netobj->setStatusPropagation(method);
         break;
      case SA_PROPAGATE_FIXED:
      case SA_PROPAGATE_RELATIVE:
         if (argc < 2)
            return NXSL_ERR_INVALID_ARGUMENT_COUNT;
         if (!argv[1]->isInteger())
            return NXSL_ERR_NOT_INTEGER;
         netobj->setStatusPropagation(method, argv[1]->getValueAsInt32());
         break;
      case SA_PROPAGATE_TRANSLATED:
         if (argc < 5)
            return NXSL_ERR_INVALID_ARGUMENT_COUNT;
         for(int i = 1; i <= 4; i++)
         {
            if (!argv[i]->isInteger())
               return NXSL_ERR_NOT_INTEGER;
         }
         netobj->setStatusPropagation(method, argv[1]->getValueAsInt32(), argv[2]->getValueAsInt32(), argv[3]->getValueAsInt32(), argv[4]->getValueAsInt32());
         break;
      default:
         success = 0;   // invalid method
         break;
   }
   *result = vm->createValue(success);
   return 0;
}

/**
 * NetObj::unbind(object)
 */
NXSL_METHOD_DEFINITION(NetObj, unbind)
{
   NetObj *thisObject = static_cast<NetObj*>(object->getData());
   if ((thisObject->getObjectClass() != OBJECT_CONTAINER) && (thisObject->getObjectClass() != OBJECT_SERVICEROOT))
      return NXSL_ERR_BAD_CLASS;

   NXSL_Object *nxslChild = argv[0]->getValueAsObject();
   if (!nxslChild->getClass()->instanceOf(g_nxslNetObjClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   NetObj *child = static_cast<NetObj*>(nxslChild->getData());
   thisObject->deleteChild(child);
   child->deleteParent(thisObject);

   *result = vm->createValue();
   return 0;
}

/**
 * NetObj::unbindFrom(object)
 */
NXSL_METHOD_DEFINITION(NetObj, unbindFrom)
{
   NetObj *thisObject = static_cast<NetObj*>(object->getData());

   NXSL_Object *nxslParent = argv[0]->getValueAsObject();
   if (!nxslParent->getClass()->instanceOf(g_nxslNetObjClass.getName()))
      return NXSL_ERR_BAD_CLASS;

   NetObj *parent = static_cast<NetObj*>(nxslParent->getData());
   if ((parent->getObjectClass() != OBJECT_CONTAINER) && (parent->getObjectClass() != OBJECT_SERVICEROOT))
      return NXSL_ERR_BAD_CLASS;

   parent->deleteChild(thisObject);
   thisObject->deleteParent(parent);

   *result = vm->createValue();
   return 0;
}

/**
 * NetObj::unmanage()
 */
NXSL_METHOD_DEFINITION(NetObj, unmanage)
{
   static_cast<NetObj*>(object->getData())->setMgmtStatus(false);
   *result = vm->createValue();
   return 0;
}

/**
 * NXSL class NetObj: constructor
 */
NXSL_NetObjClass::NXSL_NetObjClass() : NXSL_Class()
{
   setName(_T("NetObj"));

   NXSL_REGISTER_METHOD(NetObj, bind, 1);
   NXSL_REGISTER_METHOD(NetObj, bindTo, 1);
   NXSL_REGISTER_METHOD(NetObj, clearGeoLocation, 0);
   NXSL_REGISTER_METHOD(NetObj, delete, 0);
   NXSL_REGISTER_METHOD(NetObj, deleteCustomAttribute, 1);
   NXSL_REGISTER_METHOD(NetObj, enterMaintenance, -1);
   NXSL_REGISTER_METHOD(NetObj, expandString, 1);
   NXSL_REGISTER_METHOD(NetObj, getCustomAttribute, 1);
   NXSL_REGISTER_METHOD(NetObj, leaveMaintenance, 0);
   NXSL_REGISTER_METHOD(NetObj, manage, 0);
   NXSL_REGISTER_METHOD(NetObj, rename, 1);
   NXSL_REGISTER_METHOD(NetObj, setComments, 1);
   NXSL_REGISTER_METHOD(NetObj, setCustomAttribute, -1);
   NXSL_REGISTER_METHOD(NetObj, setGeoLocation, 1);
   NXSL_REGISTER_METHOD(NetObj, setMapImage, 1);
   NXSL_REGISTER_METHOD(NetObj, setStatusCalculation, -1);
   NXSL_REGISTER_METHOD(NetObj, setStatusPropagation, -1);
   NXSL_REGISTER_METHOD(NetObj, unbind, 1);
   NXSL_REGISTER_METHOD(NetObj, unbindFrom, 1);
   NXSL_REGISTER_METHOD(NetObj, unmanage, 0);
}

/**
 * Object creation handler
 */
void NXSL_NetObjClass::onObjectCreate(NXSL_Object *object)
{
   ((NetObj *)object->getData())->incRefCount();
}

/**
 * Object destruction handler
 */
void NXSL_NetObjClass::onObjectDelete(NXSL_Object *object)
{
   ((NetObj *)object->getData())->decRefCount();
}

/**
 * NXSL class NetObj: get attribute
 */
NXSL_Value *NXSL_NetObjClass::getAttr(NXSL_Object *_object, const char *attr)
{
   NXSL_VM *vm = _object->vm();
   NXSL_Value *value = NULL;
   NetObj *object = (NetObj *)_object->getData();
   if (!strcmp(attr, "alarms"))
   {
      ObjectArray<Alarm> *alarms = GetAlarms(object->getId(), true);
      alarms->setOwner(Ownership::False);
      NXSL_Array *array = new NXSL_Array(vm);
      for(int i = 0; i < alarms->size(); i++)
         array->append(vm->createValue(new NXSL_Object(vm, &g_nxslAlarmClass, alarms->get(i))));
      value = vm->createValue(array);
      delete alarms;
   }
   else if (!strcmp(attr, "backupZoneProxy"))
   {
      UINT32 id = object->getAssignedZoneProxyId(true);
      if (id != 0)
      {
         NetObj *proxy = FindObjectById(id, OBJECT_NODE);
         value = (proxy != NULL) ? proxy->createNXSLObject(vm) : vm->createValue();
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (!strcmp(attr, "backupZoneProxyId"))
   {
      value = vm->createValue(object->getAssignedZoneProxyId(true));
   }
   else if (!strcmp(attr, "children"))
   {
      value = vm->createValue(object->getChildrenForNXSL(vm));
   }
   else if (!strcmp(attr, "city"))
   {
      value = vm->createValue(object->getPostalAddress()->getCity());
   }
   else if (!strcmp(attr, "comments"))
   {
      value = vm->createValue(object->getComments());
   }
   else if (!strcmp(attr, "country"))
   {
      value = vm->createValue(object->getPostalAddress()->getCountry());
   }
   else if (!strcmp(attr, "creationTime"))
   {
      value = vm->createValue(static_cast<INT64>(object->getCreationTime()));
   }
   else if (!strcmp(attr, "customAttributes"))
   {
      value = object->getCustomAttributesForNXSL(vm);
   }
   else if (!strcmp(attr, "geolocation"))
   {
      value = NXSL_GeoLocationClass::createObject(vm, object->getGeoLocation());
   }
   else if (!strcmp(attr, "guid"))
   {
      TCHAR buffer[64];
      value = vm->createValue(object->getGuid().toString(buffer));
   }
   else if (!strcmp(attr, "id"))
   {
      value = vm->createValue(object->getId());
   }
   else if (!strcmp(attr, "ipAddr"))
   {
      TCHAR buffer[64];
      object->getPrimaryIpAddress().toString(buffer);
      value = vm->createValue(buffer);
   }
   else if (!strcmp(attr, "isInMaintenanceMode"))
   {
      value = vm->createValue(object->isInMaintenanceMode());
   }
   else if (!strcmp(attr, "mapImage"))
   {
      TCHAR buffer[64];
      value = vm->createValue(object->getMapImage().toString(buffer));
   }
   else if (!strcmp(attr, "name"))
   {
      value = vm->createValue(object->getName());
   }
   else if (!strcmp(attr, "parents"))
   {
      value = vm->createValue(object->getParentsForNXSL(vm));
   }
   else if (!strcmp(attr, "postcode"))
   {
      value = vm->createValue(object->getPostalAddress()->getPostCode());
   }
   else if (!strcmp(attr, "primaryZoneProxy"))
   {
      UINT32 id = object->getAssignedZoneProxyId(false);
      if (id != 0)
      {
         NetObj *proxy = FindObjectById(id, OBJECT_NODE);
         value = (proxy != NULL) ? proxy->createNXSLObject(vm) : vm->createValue();
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (!strcmp(attr, "primaryZoneProxyId"))
   {
      value = vm->createValue(object->getAssignedZoneProxyId(false));
   }
   else if (!strcmp(attr, "responsibleUsers"))
   {
      NXSL_Array *array = new NXSL_Array(vm);
      IntegerArray<UINT32> *responsibleUsers = object->getAllResponsibleUsers();
      ObjectArray<UserDatabaseObject> *userDB = FindUserDBObjects(responsibleUsers);
      userDB->setOwner(Ownership::False);
      for(int i = 0; i < userDB->size(); i++)
      {
         array->append(userDB->get(i)->createNXSLObject(vm));
      }
      value = vm->createValue(array);
      delete userDB;
      delete responsibleUsers;
   }
   else if (!strcmp(attr, "state"))
   {
      value = vm->createValue(object->getState());
   }
   else if (!strcmp(attr, "status"))
   {
      value = vm->createValue((LONG)object->getStatus());
   }
   else if (!strcmp(attr, "streetAddress"))
   {
      value = vm->createValue(object->getPostalAddress()->getStreetAddress());
   }
   else if (!strcmp(attr, "type"))
   {
      value = vm->createValue((LONG)object->getObjectClass());
   }
	else
	{
#ifdef UNICODE
      WCHAR wattr[MAX_IDENTIFIER_LENGTH];
      MultiByteToWideChar(CP_UTF8, 0, attr, -1, wattr, MAX_IDENTIFIER_LENGTH);
      wattr[MAX_IDENTIFIER_LENGTH - 1] = 0;
      value = object->getCustomAttributeForNXSL(vm, wattr);
#else
		value = object->getCustomAttributeForNXSL(vm, attr);
#endif
	}
   return value;
}

/**
 * NXSL class Zone: constructor
 */
NXSL_SubnetClass::NXSL_SubnetClass() : NXSL_NetObjClass()
{
   setName(_T("Subnet"));
}

/**
 * NXSL class Zone: get attribute
 */
NXSL_Value *NXSL_SubnetClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != NULL)
      return value;

   NXSL_VM *vm = object->vm();
   Subnet *subnet = static_cast<Subnet*>(object->getData());
   if (!strcmp(attr, "ipNetMask"))
   {
      value = vm->createValue(subnet->getIpAddress().getMaskBits());
   }
   else if (!strcmp(attr, "isSyntheticMask"))
   {
      value = vm->createValue(subnet->isSyntheticMask());
   }
   else if (!strcmp(attr, "zone"))
   {
      if (g_flags & AF_ENABLE_ZONING)
      {
         Zone *zone = FindZoneByUIN(subnet->getZoneUIN());
         if (zone != NULL)
         {
            value = vm->createValue(new NXSL_Object(vm, &g_nxslZoneClass, zone));
         }
         else
         {
            value = vm->createValue();
         }
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (!strcmp(attr, "zoneUIN"))
   {
      value = vm->createValue(subnet->getZoneUIN());
   }
   return value;
}

/**
 * readInternalParameter(name) method
 */
NXSL_METHOD_DEFINITION(DataCollectionTarget, readInternalParameter)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   DataCollectionTarget *dct = static_cast<DataCollectionTarget*>(object->getData());

   TCHAR value[MAX_RESULT_LENGTH];
   DataCollectionError rc = dct->getInternalItem(argv[0]->getValueAsCString(), MAX_RESULT_LENGTH, value);
   *result = (rc == DCE_SUCCESS) ? object->vm()->createValue(value) : object->vm()->createValue();
   return 0;
}

/**
 * NXSL class DataCollectionTarget: constructor
 */
NXSL_DCTargetClass::NXSL_DCTargetClass() : NXSL_NetObjClass()
{
   setName(_T("DataCollectionTarget"));

   NXSL_REGISTER_METHOD(DataCollectionTarget, readInternalParameter, 1);
}

/**
 * NXSL class Zone: get attribute
 */
NXSL_Value *NXSL_DCTargetClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != NULL)
      return value;

   NXSL_VM *vm = object->vm();
   DataCollectionTarget *dcTarget = static_cast<DataCollectionTarget*>(object->getData());
   if (!strcmp(attr, "templates"))
   {
      value = vm->createValue(dcTarget->getTemplatesForNXSL(vm));
   }
   return value;
}

/**
 * NXSL class Zone: constructor
 */
NXSL_ZoneClass::NXSL_ZoneClass() : NXSL_NetObjClass()
{
   setName(_T("Zone"));
}

/**
 * NXSL class Zone: get attribute
 */
NXSL_Value *NXSL_ZoneClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != NULL)
      return value;

   NXSL_VM *vm = object->vm();
   Zone *zone = static_cast<Zone*>(object->getData());
   if (!strcmp(attr, "proxyNodes"))
   {
      NXSL_Array *array = new NXSL_Array(vm);
      IntegerArray<UINT32> *proxies = zone->getAllProxyNodes();
      for(int i = 0; i < proxies->size(); i++)
      {
         Node *node = static_cast<Node*>(FindObjectById(proxies->get(i), OBJECT_NODE));
         if (node != NULL)
            array->append(node->createNXSLObject(vm));
      }
      value = vm->createValue(array);
   }
   else if (!strcmp(attr, "proxyNodeIds"))
   {
      NXSL_Array *array = new NXSL_Array(vm);
      IntegerArray<UINT32> *proxies = zone->getAllProxyNodes();
      for(int i = 0; i < proxies->size(); i++)
         array->append(vm->createValue(proxies->get(i)));
      value = vm->createValue(array);
   }
   else if (!strcmp(attr, "uin"))
   {
      value = vm->createValue(zone->getUIN());
   }
   else if (!strcmp(attr, "snmpPorts"))
   {
      value = vm->createValue(new NXSL_Array(vm, zone->getSnmpPortList()));
   }
   return value;
}

/**
 * Generic implementation for flag changing methods
 */
static int ChangeFlagMethod(NXSL_Object *object, NXSL_Value *arg, NXSL_Value **result, UINT32 flag, bool invert)
{
   if (!arg->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   Node *node = static_cast<Node*>(object->getData());
   if (arg->isTrue())
   {
      if (invert)
         node->clearFlag(flag);
      else
         node->setFlag(flag);
   }
   else
   {
      if (invert)
         node->setFlag(flag);
      else
         node->clearFlag(flag);
   }

   *result = object->vm()->createValue();
   return 0;
}

/**
 * Node::createSNMPTransport(port, context) method
 */
NXSL_METHOD_DEFINITION(Node, createSNMPTransport)
{
   if (argc > 2)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if ((argc > 0) && !argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   if ((argc > 1) && !argv[1]->isString())
      return NXSL_ERR_NOT_STRING;

   UINT16 port = (argc > 0) ? static_cast<UINT16>(argv[0]->getValueAsInt32()) : 0;
   const TCHAR *context = (argc > 1) ? argv[1]->getValueAsCString() : NULL;
   SNMP_Transport *t = static_cast<Node*>(object->getData())->createSnmpTransport(port, SNMP_VERSION_DEFAULT, context);
   *result = (t != NULL) ? vm->createValue(new NXSL_Object(vm, &g_nxslSnmpTransportClass, t)) : vm->createValue();
   return 0;
}

/**
 * enableAgent(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableAgent)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_NXCP, true);
}

/**
 * enableConfigurationPolling(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableConfigurationPolling)
{
   return ChangeFlagMethod(object, argv[0], result, DCF_DISABLE_CONF_POLL, true);
}

/**
 * enableDiscoveryPolling(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableDiscoveryPolling)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_DISCOVERY_POLL, true);
}

/**
 * enableEtherNetIP(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableEtherNetIP)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_ETHERNET_IP, true);
}

/**
 * enableIcmp(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableIcmp)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_ICMP, true);
}

/**
 * enablePrimaryIPPing(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enablePrimaryIPPing)
{
   return ChangeFlagMethod(object, argv[0], result, NF_PING_PRIMARY_IP, false);
}

/**
 * enableRoutingTablePolling(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableRoutingTablePolling)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_ROUTE_POLL, true);
}

/**
 * enableSnmp(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableSnmp)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_SNMP, true);
}

/**
 * enableStatusPolling(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableStatusPolling)
{
   return ChangeFlagMethod(object, argv[0], result, DCF_DISABLE_STATUS_POLL, true);
}

/**
 * enableTopologyPolling(enabled) method
 */
NXSL_METHOD_DEFINITION(Node, enableTopologyPolling)
{
   return ChangeFlagMethod(object, argv[0], result, NF_DISABLE_TOPOLOGY_POLL, true);
}

/**
 * Node::executeSSHCommand(command) method
 */
NXSL_METHOD_DEFINITION(Node, executeSSHCommand)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   Node *node = static_cast<Node*>(object->getData());
   UINT32 proxyId = node->getEffectiveSshProxy();
   if (proxyId != 0)
   {
      Node *proxyNode = static_cast<Node*>(FindObjectById(proxyId, OBJECT_NODE));
      if (proxyNode != NULL)
      {
         TCHAR command[MAX_PARAM_NAME], ipAddr[64];
         _sntprintf(command, MAX_PARAM_NAME, _T("SSH.Command(%s,\"%s\",\"%s\",\"%s\")"),
                    node->getIpAddress().toString(ipAddr),
                    (const TCHAR *)EscapeStringForAgent(node->getSshLogin()),
                    (const TCHAR *)EscapeStringForAgent(node->getSshPassword()),
                    (const TCHAR *)EscapeStringForAgent(argv[0]->getValueAsCString()));
         StringList *list;
         UINT32 rcc = proxyNode->getListFromAgent(command, &list);
         *result = (rcc == DCE_SUCCESS) ? vm->createValue(new NXSL_Array(vm, list)) : vm->createValue();
         delete list;
      }
      else
      {
         *result = vm->createValue();
      }
   }
   else
   {
      *result = vm->createValue();
   }
   return 0;
}

/**
 * Node::getInterface(ifIndex) method
 */
NXSL_METHOD_DEFINITION(Node, getInterface)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   Interface *iface = static_cast<Node*>(object->getData())->findInterfaceByIndex(argv[0]->getValueAsUInt32());
   *result = (iface != NULL) ? iface->createNXSLObject(vm) : vm->createValue();
   return 0;
}

/**
 * Node::getInterfaceName(ifIndex) method
 */
NXSL_METHOD_DEFINITION(Node, getInterfaceName)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   Interface *iface = static_cast<Node*>(object->getData())->findInterfaceByIndex(argv[0]->getValueAsUInt32());
   *result = (iface != NULL) ? vm->createValue(iface->getName()) : vm->createValue();
   return 0;
}

/**
 * Node::readAgentParameter(name) method
 */
NXSL_METHOD_DEFINITION(Node, readAgentParameter)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   TCHAR buffer[MAX_RESULT_LENGTH];
   UINT32 rcc = static_cast<Node*>(object->getData())->getItemFromAgent(argv[0]->getValueAsCString(), MAX_RESULT_LENGTH, buffer);
   *result = (rcc == DCE_SUCCESS) ? vm->createValue(buffer) : vm->createValue();
   return 0;
}

/**
 * Node::readAgentList(name) method
 */
NXSL_METHOD_DEFINITION(Node, readAgentList)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   StringList *list;
   UINT32 rcc = static_cast<Node*>(object->getData())->getListFromAgent(argv[0]->getValueAsCString(), &list);
   *result = (rcc == DCE_SUCCESS) ? vm->createValue(new NXSL_Array(vm, list)) : vm->createValue();
   delete list;
   return 0;
}

/**
 * Node::readDriverParameter(name) method
 */
NXSL_METHOD_DEFINITION(Node, readDriverParameter)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   TCHAR buffer[MAX_RESULT_LENGTH];
   UINT32 rcc = static_cast<Node*>(object->getData())->getItemFromDeviceDriver(argv[0]->getValueAsCString(), buffer, MAX_RESULT_LENGTH);
   *result = (rcc == DCE_SUCCESS) ? vm->createValue(buffer) : vm->createValue();
   return 0;
}

/**
 * Node::readAgentTable(name) method
 */
NXSL_METHOD_DEFINITION(Node, readAgentTable)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   Table *table;
   UINT32 rcc = static_cast<Node*>(object->getData())->getTableFromAgent(argv[0]->getValueAsCString(), &table);
   *result = (rcc == DCE_SUCCESS) ? vm->createValue(new NXSL_Object(vm, &g_nxslTableClass, table)) : vm->createValue();
   return 0;
}

/**
 * NXSL class Node: constructor
 */
NXSL_NodeClass::NXSL_NodeClass() : NXSL_DCTargetClass()
{
   setName(_T("Node"));

   NXSL_REGISTER_METHOD(Node, createSNMPTransport, -1);
   NXSL_REGISTER_METHOD(Node, enableAgent, 1);
   NXSL_REGISTER_METHOD(Node, enableConfigurationPolling, 1);
   NXSL_REGISTER_METHOD(Node, enableDiscoveryPolling, 1);
   NXSL_REGISTER_METHOD(Node, enableEtherNetIP, 1);
   NXSL_REGISTER_METHOD(Node, enableIcmp, 1);
   NXSL_REGISTER_METHOD(Node, enablePrimaryIPPing, 1);
   NXSL_REGISTER_METHOD(Node, enableRoutingTablePolling, 1);
   NXSL_REGISTER_METHOD(Node, enableSnmp, 1);
   NXSL_REGISTER_METHOD(Node, enableStatusPolling, 1);
   NXSL_REGISTER_METHOD(Node, enableTopologyPolling, 1);
   NXSL_REGISTER_METHOD(Node, executeSSHCommand, 1);
   NXSL_REGISTER_METHOD(Node, getInterface, 1);
   NXSL_REGISTER_METHOD(Node, getInterfaceName, 1);
   NXSL_REGISTER_METHOD(Node, readAgentList, 1);
   NXSL_REGISTER_METHOD(Node, readAgentParameter, 1);
   NXSL_REGISTER_METHOD(Node, readAgentTable, 1);
   NXSL_REGISTER_METHOD(Node, readDriverParameter, 1);
}

/**
 * Get ICMP statistic for object
 */
static NXSL_Value *GetNodeIcmpStatistic(Node *node, IcmpStatFunction function, NXSL_VM *vm)
{
   NXSL_Value *value;
   TCHAR buffer[MAX_RESULT_LENGTH];
   if (node->getIcmpStatistic(NULL, function, buffer) == DCE_SUCCESS)
   {
      value = vm->createValue(buffer);
   }
   else
   {
      value = vm->createValue();
   }
   return value;
}

/**
 * NXSL class Node: get attribute
 */
NXSL_Value *NXSL_NodeClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_DCTargetClass::getAttr(object, attr);
   if (value != NULL)
      return value;

   NXSL_VM *vm = object->vm();
   Node *node = (Node *)object->getData();
   if (!strcmp(attr, "agentCertificateSubject"))
   {
      value = vm->createValue(node->getAgentCertificateSubject());
   }
   else if (!strcmp(attr, "agentId"))
   {
      TCHAR buffer[64];
      value = vm->createValue(node->getAgentId().toString(buffer));
   }
   else if (!strcmp(attr, "agentVersion"))
   {
      value = vm->createValue(node->getAgentVersion());
   }
   else if (!strcmp(attr, "bootTime"))
   {
      value = vm->createValue(static_cast<INT64>(node->getBootTime()));
   }
   else if (!strcmp(attr, "bridgeBaseAddress"))
   {
      TCHAR buffer[64];
      value = vm->createValue(BinToStr(node->getBridgeId(), MAC_ADDR_LENGTH, buffer));
   }
   else if (!strcmp(attr, "capabilities"))
   {
      value = vm->createValue(node->getCapabilities());
   }
   else if (!strcmp(attr, "cipDeviceType"))
   {
      value = vm->createValue(node->getCipDeviceType());
   }
   else if (!strcmp(attr, "cipDeviceTypeAsText"))
   {
      value = vm->createValue(CIP_DeviceTypeNameFromCode(node->getCipDeviceType()));
   }
   else if (!strcmp(attr, "cipExtendedStatus"))
   {
      value = vm->createValue((node->getCipStatus() & CIP_DEVICE_STATUS_EXTENDED_STATUS_MASK) >> 4);
   }
   else if (!strcmp(attr, "cipExtendedStatusAsText"))
   {
      value = vm->createValue(CIP_DecodeExtendedDeviceStatus(node->getCipStatus()));
   }
   else if (!strcmp(attr, "cipStatus"))
   {
      value = vm->createValue(node->getCipStatus());
   }
   else if (!strcmp(attr, "cipStatusAsText"))
   {
      value = vm->createValue(CIP_DecodeDeviceStatus(node->getCipStatus()));
   }
   else if (!strcmp(attr, "cipState"))
   {
      value = vm->createValue(node->getCipState());
   }
   else if (!strcmp(attr, "cipStateAsText"))
   {
      value = vm->createValue(CIP_DeviceStateTextFromCode(node->getCipState()));
   }
   else if (!strcmp(attr, "components"))
   {
      shared_ptr<ComponentTree> components = node->getComponents();
      if (components != NULL)
      {
         value = ComponentTree::getRootForNXSL(vm, components);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (!strcmp(attr, "dependentNodes"))
   {
      StructArray<DependentNode> *dependencies = GetNodeDependencies(node->getId());
      NXSL_Array *a = new NXSL_Array(vm);
      for(int i = 0; i < dependencies->size(); i++)
      {
         a->append(vm->createValue(new NXSL_Object(vm, &g_nxslNodeDependencyClass, new DependentNode(*dependencies->get(i)))));
      }
      value = vm->createValue(a);
   }
   else if (!strcmp(attr, "driver"))
   {
      value = vm->createValue(node->getDriverName());
   }
   else if (!strcmp(attr, "downSince"))
   {
      value = vm->createValue(static_cast<INT64>(node->getDownSince()));
   }
   else if (!strcmp(attr, "flags"))
   {
		value = vm->createValue(node->getFlags());
   }
   else if (!strcmp(attr, "hasAgentIfXCounters"))
   {
      value = vm->createValue((node->getCapabilities() & NC_HAS_AGENT_IFXCOUNTERS) ? 1 : 0);
   }
   else if (!strcmp(attr, "hasEntityMIB"))
   {
      value = vm->createValue((node->getCapabilities() & NC_HAS_ENTITY_MIB) ? 1 : 0);
   }
   else if (!strcmp(attr, "hasIfXTable"))
   {
      value = vm->createValue((node->getCapabilities() & NC_HAS_IFXTABLE) ? 1 : 0);
   }
   else if (!strcmp(attr, "hasUserAgent"))
   {
      value = vm->createValue((LONG)((node->getCapabilities() & NC_HAS_USER_AGENT) ? 1 : 0));
   }
   else if (!strcmp(attr, "hasVLANs"))
   {
      value = vm->createValue((node->getCapabilities() & NC_HAS_VLANS) ? 1 : 0);
   }
   else if (!strcmp(attr, "hasWinPDH"))
   {
      value = vm->createValue((node->getCapabilities() & NC_HAS_WINPDH) ? 1 : 0);
   }
   else if (!strcmp(attr, "hypervisorInfo"))
   {
      value = vm->createValue(node->getHypervisorInfo());
   }
   else if (!strcmp(attr, "hypervisorType"))
   {
      value = vm->createValue(node->getHypervisorType());
   }
   else if (!strcmp(attr, "icmpAverageRTT"))
   {
      value = GetNodeIcmpStatistic(node, IcmpStatFunction::AVERAGE, vm);
   }
   else if (!strcmp(attr, "icmpLastRTT"))
   {
      value = GetNodeIcmpStatistic(node, IcmpStatFunction::LAST, vm);
   }
   else if (!strcmp(attr, "icmpMaxRTT"))
   {
      value = GetNodeIcmpStatistic(node, IcmpStatFunction::MAX, vm);
   }
   else if (!strcmp(attr, "icmpMinRTT"))
   {
      value = GetNodeIcmpStatistic(node, IcmpStatFunction::MIN, vm);
   }
   else if (!strcmp(attr, "icmpPacketLoss"))
   {
      value = GetNodeIcmpStatistic(node, IcmpStatFunction::LOSS, vm);
   }
   else if (!strcmp(attr, "interfaces"))
   {
      value = vm->createValue(node->getInterfacesForNXSL(vm));
   }
   else if (!strcmp(attr, "isAgent"))
   {
      value = vm->createValue((LONG)((node->getCapabilities() & NC_IS_NATIVE_AGENT) ? 1 : 0));
   }
   else if (!strcmp(attr, "isBridge"))
   {
      value = vm->createValue((LONG)((node->getCapabilities() & NC_IS_BRIDGE) ? 1 : 0));
   }
   else if (!strcmp(attr, "isCDP"))
   {
      value = vm->createValue((LONG)((node->getCapabilities() & NC_IS_CDP) ? 1 : 0));
   }
   else if (!strcmp(attr, "isEtherNetIP"))
   {
      value = vm->createValue((LONG)((node->getCapabilities() & NC_IS_ETHERNET_IP) ? 1 : 0));
   }
   else if (!strcmp(attr, "isInMaintenanceMode"))
   {
      value = vm->createValue((LONG)(node->isInMaintenanceMode() ? 1 : 0));
   }
   else if (!strcmp(attr, "isLLDP"))
   {
      value = vm->createValue((LONG)((node->getCapabilities() & NC_IS_LLDP) ? 1 : 0));
   }
	else if (!strcmp(attr, "isLocalMgmt") || !strcmp(attr, "isLocalManagement"))
	{
		value = vm->createValue((LONG)((node->isLocalManagement()) ? 1 : 0));
	}
   else if (!strcmp(attr, "isModbusTCP"))
   {
      value = vm->createValue((LONG)((node->getCapabilities() & NC_IS_MODBUS_TCP) ? 1 : 0));
   }
   else if (!strcmp(attr, "isOSPF"))
   {
      value = vm->createValue((LONG)((node->getCapabilities() & NC_IS_OSPF) ? 1 : 0));
   }
   else if (!strcmp(attr, "isPAE") || !strcmp(attr, "is802_1x"))
   {
      value = vm->createValue((LONG)((node->getCapabilities() & NC_IS_8021X) ? 1 : 0));
   }
   else if (!strcmp(attr, "isPrinter"))
   {
      value = vm->createValue((LONG)((node->getCapabilities() & NC_IS_PRINTER) ? 1 : 0));
   }
   else if (!strcmp(attr, "isProfiNet"))
   {
      value = vm->createValue((LONG)((node->getCapabilities() & NC_IS_PROFINET) ? 1 : 0));
   }
   else if (!strcmp(attr, "isRouter"))
   {
      value = vm->createValue((LONG)((node->getCapabilities() & NC_IS_ROUTER) ? 1 : 0));
   }
   else if (!strcmp(attr, "isSMCLP"))
   {
      value = vm->createValue((LONG)((node->getCapabilities() & NC_IS_SMCLP) ? 1 : 0));
   }
   else if (!strcmp(attr, "isSNMP"))
   {
      value = vm->createValue((LONG)((node->getCapabilities() & NC_IS_SNMP) ? 1 : 0));
   }
   else if (!strcmp(attr, "isSONMP") || !strcmp(attr, "isNDP"))
   {
      value = vm->createValue((LONG)((node->getCapabilities() & NC_IS_NDP) ? 1 : 0));
   }
   else if (!strcmp(attr, "isSTP"))
   {
      value = vm->createValue((LONG)((node->getCapabilities() & NC_IS_STP) ? 1 : 0));
   }
   else if (!strcmp(attr, "isVirtual"))
   {
      value = vm->createValue((LONG)(node->isVirtual() ? 1 : 0));
   }
   else if (!strcmp(attr, "isVRRP"))
   {
      value = vm->createValue((node->getCapabilities() & NC_IS_VRRP) ? 1 : 0);
   }
   else if (!strcmp(attr, "lastAgentCommTime"))
   {
      value = vm->createValue((INT64)node->getLastAgentCommTime());
   }
   else if (!strcmp(attr, "nodeSubType"))
   {
      value = vm->createValue(node->getSubType());
   }
   else if (!strcmp(attr, "nodeType"))
   {
      value = vm->createValue((INT32)node->getType());
   }
   else if (!strcmp(attr, "physicalContainer"))
   {
      NetObj *object = FindObjectById(node->getPhysicalContainerId());
      if (object != NULL)
      {
         value = object->createNXSLObject(vm);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (!strcmp(attr, "physicalContainerId"))
   {
      value = vm->createValue(node->getPhysicalContainerId());
   }
   else if (!strcmp(attr, "platformName"))
   {
      value = vm->createValue(node->getPlatformName());
   }
   else if (!strcmp(attr, "productCode"))
   {
      value = vm->createValue(node->getProductCode());
   }
   else if (!strcmp(attr, "productName"))
   {
      value = vm->createValue(node->getProductName());
   }
   else if (!strcmp(attr, "productVersion"))
   {
      value = vm->createValue(node->getProductVersion());
   }
   else if (!strcmp(attr, "rack"))
   {
      Rack *rack = (Rack *)FindObjectById(node->getPhysicalContainerId(), OBJECT_RACK);
      if (rack != NULL)
      {
         value = rack->createNXSLObject(vm);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (!strcmp(attr, "rackId"))
   {
      Rack *rack = (Rack *)FindObjectById(node->getPhysicalContainerId(), OBJECT_RACK);
      if (rack != NULL)
      {
         value = vm->createValue(node->getPhysicalContainerId());
      }
      else
         value = 0;
   }
   else if (!strcmp(attr, "rackHeight"))
   {
      value = vm->createValue(node->getRackHeight());
   }
   else if (!strcmp(attr, "rackPosition"))
   {
      value = vm->createValue(node->getRackPosition());
   }
   else if (!strcmp(attr, "runtimeFlags"))
   {
      value = vm->createValue(node->getRuntimeFlags());
   }
   else if (!strcmp(attr, "serialNumber"))
   {
      value = vm->createValue(node->getSerialNumber());
   }
   else if (!strcmp(attr, "snmpOID"))
   {
      value = vm->createValue(node->getSNMPObjectId());
   }
   else if (!strcmp(attr, "snmpSysContact"))
   {
      value = vm->createValue(node->getSysContact());
   }
   else if (!strcmp(attr, "snmpSysLocation"))
   {
      value = vm->createValue(node->getSysLocation());
   }
   else if (!strcmp(attr, "snmpSysName"))
   {
      value = vm->createValue(node->getSysName());
   }
   else if (!strcmp(attr, "snmpVersion"))
   {
      value = vm->createValue((LONG)node->getSNMPVersion());
   }
   else if (!strcmp(attr, "sysDescription"))
   {
      value = vm->createValue(node->getSysDescription());
   }
   else if (!strcmp(attr, "vendor"))
   {
      value = vm->createValue(node->getVendor());
   }
   else if (!strcmp(attr, "vlans"))
   {
      shared_ptr<VlanList> vlans = node->getVlans();
      if (vlans != NULL)
      {
         NXSL_Array *a = new NXSL_Array(vm);
         for(int i = 0; i < vlans->size(); i++)
         {
            a->append(vm->createValue(new NXSL_Object(vm, &g_nxslVlanClass, new VlanInfo(vlans->get(i), node->getId()))));
         }
         value = vm->createValue(a);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (!strcmp(attr, "zone"))
	{
      if (IsZoningEnabled())
      {
         Zone *zone = FindZoneByUIN(node->getZoneUIN());
		   if (zone != NULL)
		   {
			   value = vm->createValue(new NXSL_Object(vm, &g_nxslZoneClass, zone));
		   }
		   else
		   {
			   value = vm->createValue();
		   }
	   }
	   else
	   {
		   value = vm->createValue();
	   }
	}
   else if (!strcmp(attr, "zoneProxyAssignments"))
   {
      if (IsZoningEnabled())
      {
         Zone *zone = FindZoneByProxyId(node->getId());
         if (zone != NULL)
         {
            value = vm->createValue(zone->getProxyNodeAssignments(node->getId()));
         }
         else
         {
            value = vm->createValue(0);
         }
      }
      else
      {
         value = vm->createValue(0);
      }
   }
   else if (!strcmp(attr, "zoneProxyStatus"))
   {
      if (IsZoningEnabled())
      {
         Zone *zone = FindZoneByProxyId(node->getId());
         if (zone != NULL)
         {
            value = vm->createValue(zone->isProxyNodeAvailable(node->getId()));
         }
         else
         {
            value = vm->createValue(0);
         }
      }
      else
      {
         value = vm->createValue(0);
      }
   }
   else if (!strcmp(attr, "zoneUIN"))
	{
      value = vm->createValue(node->getZoneUIN());
   }
   return value;
}

/**
 * Interface::setExcludeFromTopology(enabled) method
 */
NXSL_METHOD_DEFINITION(Interface, setExcludeFromTopology)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   Interface *iface = static_cast<Interface*>(object->getData());
   iface->setExcludeFromTopology(argv[0]->getValueAsBoolean());
   *result = vm->createValue();
   return 0;
}

/**
 * Interface::setExpectedState(state) method
 */
NXSL_METHOD_DEFINITION(Interface, setExpectedState)
{
   int state;
   if (argv[0]->isInteger())
   {
      state = argv[0]->getValueAsInt32();
   }
   else if (argv[0]->isString())
   {
      static const TCHAR *stateNames[] = { _T("UP"), _T("DOWN"), _T("IGNORE"), NULL };
      const TCHAR *name = argv[0]->getValueAsCString();
      for(state = 0; stateNames[state] != NULL; state++)
         if (!_tcsicmp(stateNames[state], name))
            break;
   }
   else
   {
      return NXSL_ERR_NOT_STRING;
   }

   if ((state >= 0) && (state <= 2))
      static_cast<Interface*>(object->getData())->setExpectedState(state);

   *result = vm->createValue();
   return 0;
}

/**
 * Interface::setIncludeInIcmpPoll(enabled) method
 */
NXSL_METHOD_DEFINITION(Interface, setIncludeInIcmpPoll)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   Interface *iface = static_cast<Interface*>(object->getData());
   iface->setIncludeInIcmpPoll(argv[0]->getValueAsBoolean());
   *result = vm->createValue();
   return 0;
}

/**
 * NXSL class Interface: constructor
 */
NXSL_InterfaceClass::NXSL_InterfaceClass() : NXSL_NetObjClass()
{
   setName(_T("Interface"));

   NXSL_REGISTER_METHOD(Interface, setExcludeFromTopology, 1);
   NXSL_REGISTER_METHOD(Interface, setExpectedState, 1);
   NXSL_REGISTER_METHOD(Interface, setIncludeInIcmpPoll, 1);
}

/**
 * NXSL class Interface: get attribute
 */
NXSL_Value *NXSL_InterfaceClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != NULL)
      return value;

   NXSL_VM *vm = object->vm();
   Interface *iface = static_cast<Interface*>(object->getData());
   if (!strcmp(attr, "adminState"))
   {
		value = vm->createValue((LONG)iface->getAdminState());
   }
   else if (!strcmp(attr, "alias"))
   {
      value = vm->createValue(iface->getAlias());
   }
   else if (!strcmp(attr, "bridgePortNumber"))
   {
		value = vm->createValue(iface->getBridgePortNumber());
   }
   else if (!strcmp(attr, "chassis"))
   {
      value = vm->createValue(iface->getChassis());
   }
   else if (!strcmp(attr, "description"))
   {
      value = vm->createValue(iface->getDescription());
   }
   else if (!strcmp(attr, "dot1xBackendAuthState"))
   {
		value = vm->createValue((LONG)iface->getDot1xBackendAuthState());
   }
   else if (!strcmp(attr, "dot1xPaeAuthState"))
   {
		value = vm->createValue((LONG)iface->getDot1xPaeAuthState());
   }
   else if (!strcmp(attr, "expectedState"))
   {
		value = vm->createValue((iface->getFlags() & IF_EXPECTED_STATE_MASK) >> 28);
   }
   else if (!strcmp(attr, "flags"))
   {
		value = vm->createValue(iface->getFlags());
   }
   else if (!strcmp(attr, "icmpAverageRTT"))
   {
      value = GetObjectIcmpStatistic(iface, IcmpStatFunction::AVERAGE, vm);
   }
   else if (!strcmp(attr, "icmpLastRTT"))
   {
      value = GetObjectIcmpStatistic(iface, IcmpStatFunction::LAST, vm);
   }
   else if (!strcmp(attr, "icmpMaxRTT"))
   {
      value = GetObjectIcmpStatistic(iface, IcmpStatFunction::MAX, vm);
   }
   else if (!strcmp(attr, "icmpMinRTT"))
   {
      value = GetObjectIcmpStatistic(iface, IcmpStatFunction::MIN, vm);
   }
   else if (!strcmp(attr, "icmpPacketLoss"))
   {
      value = GetObjectIcmpStatistic(iface, IcmpStatFunction::LOSS, vm);
   }
   else if (!strcmp(attr, "ifIndex"))
   {
		value = vm->createValue(iface->getIfIndex());
   }
   else if (!strcmp(attr, "ifType"))
   {
		value = vm->createValue(iface->getIfType());
   }
   else if (!strcmp(attr, "ipAddressList"))
   {
      const InetAddressList *addrList = iface->getIpAddressList();
      NXSL_Array *a = new NXSL_Array(vm);
      for(int i = 0; i < addrList->size(); i++)
      {
         a->append(NXSL_InetAddressClass::createObject(vm, addrList->get(i)));
      }
      value = vm->createValue(a);
   }
   else if (!strcmp(attr, "isExcludedFromTopology"))
   {
      value = vm->createValue((LONG)(iface->isExcludedFromTopology() ? 1 : 0));
   }
   else if (!strcmp(attr, "isIncludedInIcmpPoll"))
   {
      value = vm->createValue((LONG)(iface->isIncludedInIcmpPoll() ? 1 : 0));
   }
   else if (!strcmp(attr, "isLoopback"))
   {
		value = vm->createValue((LONG)(iface->isLoopback() ? 1 : 0));
   }
   else if (!strcmp(attr, "isManuallyCreated"))
   {
		value = vm->createValue((LONG)(iface->isManuallyCreated() ? 1 : 0));
   }
   else if (!strcmp(attr, "isPhysicalPort"))
   {
		value = vm->createValue((LONG)(iface->isPhysicalPort() ? 1 : 0));
   }
   else if (!strcmp(attr, "macAddr"))
   {
		TCHAR buffer[256];
		value = vm->createValue(iface->getMacAddr().toString(buffer));
   }
   else if (!strcmp(attr, "module"))
   {
      value = vm->createValue(iface->getModule());
   }
   else if (!strcmp(attr, "mtu"))
   {
      value = vm->createValue(iface->getMTU());
   }
   else if (!strcmp(attr, "node"))
	{
		Node *parentNode = iface->getParentNode();
		if (parentNode != NULL)
		{
         value = parentNode->createNXSLObject(vm);
		}
		else
		{
			value = vm->createValue();
		}
	}
   else if (!strcmp(attr, "operState"))
   {
		value = vm->createValue((LONG)iface->getOperState());
   }
   else if (!strcmp(attr, "peerInterface"))
   {
		Interface *peerIface = (Interface *)FindObjectById(iface->getPeerInterfaceId(), OBJECT_INTERFACE);
		if (peerIface != NULL)
		{
			if (g_flags & AF_CHECK_TRUSTED_NODES)
			{
				Node *parentNode = iface->getParentNode();
				Node *peerNode = peerIface->getParentNode();
				if ((parentNode != NULL) && (peerNode != NULL))
				{
					if (peerNode->isTrustedNode(parentNode->getId()))
					{
						value = vm->createValue(new NXSL_Object(vm, &g_nxslInterfaceClass, peerIface));
					}
					else
					{
						// No access, return null
						value = vm->createValue();
						DbgPrintf(4, _T("NXSL::Interface::peerInterface(%s [%d]): access denied for node %s [%d]"),
									 iface->getName(), iface->getId(), peerNode->getName(), peerNode->getId());
					}
				}
				else
				{
					value = vm->createValue();
					DbgPrintf(4, _T("NXSL::Interface::peerInterface(%s [%d]): parentNode=%p peerNode=%p"), iface->getName(), iface->getId(), parentNode, peerNode);
				}
			}
			else
			{
				value = vm->createValue(new NXSL_Object(vm, &g_nxslInterfaceClass, peerIface));
			}
		}
		else
		{
			value = vm->createValue();
		}
   }
   else if (!strcmp(attr, "peerNode"))
   {
		Node *peerNode = (Node *)FindObjectById(iface->getPeerNodeId(), OBJECT_NODE);
		if (peerNode != NULL)
		{
			if (g_flags & AF_CHECK_TRUSTED_NODES)
			{
				Node *parentNode = iface->getParentNode();
				if ((parentNode != NULL) && (peerNode->isTrustedNode(parentNode->getId())))
				{
					value = vm->createValue(new NXSL_Object(vm, &g_nxslNodeClass, peerNode));
				}
				else
				{
					// No access, return null
					value = vm->createValue();
					DbgPrintf(4, _T("NXSL::Interface::peerNode(%s [%d]): access denied for node %s [%d]"),
					          iface->getName(), iface->getId(), peerNode->getName(), peerNode->getId());
				}
			}
			else
			{
				value = vm->createValue(new NXSL_Object(vm, &g_nxslNodeClass, peerNode));
			}
		}
		else
		{
			value = vm->createValue();
		}
   }
   else if (!strcmp(attr, "pic"))
   {
      value = vm->createValue(iface->getPIC());
   }
   else if (!strcmp(attr, "port"))
   {
      value = vm->createValue(iface->getPort());
   }
   else if (!strcmp(attr, "speed"))
   {
      value = vm->createValue(iface->getSpeed());
   }
   else if (!strcmp(attr, "vlans"))
   {
      value = iface->getVlanListForNXSL(vm);
   }
   else if (!strcmp(attr, "zone"))
	{
      if (g_flags & AF_ENABLE_ZONING)
      {
         Zone *zone = FindZoneByUIN(iface->getZoneUIN());
		   if (zone != NULL)
		   {
			   value = vm->createValue(new NXSL_Object(vm, &g_nxslZoneClass, zone));
		   }
		   else
		   {
			   value = vm->createValue();
		   }
	   }
	   else
	   {
		   value = vm->createValue();
	   }
	}
   else if (!strcmp(attr, "zoneUIN"))
	{
      value = vm->createValue(iface->getZoneUIN());
   }
   return value;
}

/**
 * NXSL class AccessPoint: constructor
 */
NXSL_AccessPointClass::NXSL_AccessPointClass() : NXSL_DCTargetClass()
{
   setName(_T("AccessPoint"));
}

/**
 * NXSL class AccessPoint: get attribute
 */
NXSL_Value *NXSL_AccessPointClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_DCTargetClass::getAttr(object, attr);
   if (value != NULL)
      return value;

   NXSL_VM *vm = object->vm();
   AccessPoint *ap = static_cast<AccessPoint*>(object->getData());
   if (!strcmp(attr, "icmpAverageRTT"))
   {
      value = GetObjectIcmpStatistic(ap, IcmpStatFunction::AVERAGE, vm);
   }
   else if (!strcmp(attr, "icmpLastRTT"))
   {
      value = GetObjectIcmpStatistic(ap, IcmpStatFunction::LAST, vm);
   }
   else if (!strcmp(attr, "icmpMaxRTT"))
   {
      value = GetObjectIcmpStatistic(ap, IcmpStatFunction::MAX, vm);
   }
   else if (!strcmp(attr, "icmpMinRTT"))
   {
      value = GetObjectIcmpStatistic(ap, IcmpStatFunction::MIN, vm);
   }
   else if (!strcmp(attr, "icmpPacketLoss"))
   {
      value = GetObjectIcmpStatistic(ap, IcmpStatFunction::LOSS, vm);
   }
   else if (!strcmp(attr, "index"))
   {
      value = vm->createValue(ap->getIndex());
   }
   else if (!strcmp(attr, "model"))
   {
      value = vm->createValue(ap->getModel());
   }
   else if (!strcmp(attr, "node"))
   {
      Node *parentNode = ap->getParentNode();
      if (parentNode != NULL)
      {
         value = parentNode->createNXSLObject(vm);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (!strcmp(attr, "serialNumber"))
   {
      value = vm->createValue(ap->getSerialNumber());
   }
   else if (!strcmp(attr, "state"))
   {
      value = vm->createValue(ap->getApState());
   }
   else if (!strcmp(attr, "vendor"))
   {
      value = vm->createValue(ap->getVendor());
   }
   return value;
}

/**
 * NXSL class Mobile Device: constructor
 */
NXSL_MobileDeviceClass::NXSL_MobileDeviceClass() : NXSL_DCTargetClass()
{
   setName(_T("MobileDevice"));
}

/**
 * NXSL class Mobile Device: get attribute
 */
NXSL_Value *NXSL_MobileDeviceClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_DCTargetClass::getAttr(object, attr);
   if (value != NULL)
      return value;

   NXSL_VM *vm = object->vm();
   MobileDevice *mobDevice = (MobileDevice *)object->getData();
   if (!strcmp(attr, "deviceId"))
   {
		value = vm->createValue(mobDevice->getDeviceId());
   }
   else if (!strcmp(attr, "vendor"))
   {
      value = vm->createValue(mobDevice->getVendor());
   }
   else if (!strcmp(attr, "model"))
   {
      value = vm->createValue(mobDevice->getModel());
   }
   else if (!strcmp(attr, "serialNumber"))
   {
      value = vm->createValue(mobDevice->getSerialNumber());
   }
   else if (!strcmp(attr, "osName"))
   {
      value = vm->createValue(mobDevice->getOsName());
   }
   else if (!strcmp(attr, "osVersion"))
   {
      value = vm->createValue(mobDevice->getOsVersion());
   }
   else if (!strcmp(attr, "userId"))
   {
      value = vm->createValue(mobDevice->getUserId());
   }
   else if (!strcmp(attr, "batteryLevel"))
   {
      value = vm->createValue(mobDevice->getBatteryLevel());
   }

   return value;
}

/**
 * NXSL class "Chassis" constructor
 */
NXSL_ChassisClass::NXSL_ChassisClass() : NXSL_NetObjClass()
{
   setName(_T("Chassis"));
}

/**
 * NXSL class "Chassis" attributes
 */
NXSL_Value *NXSL_ChassisClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != NULL)
      return value;

   NXSL_VM *vm = object->vm();
   Chassis *chassis = (Chassis *)object->getData();
   if (!strcmp(attr, "controller"))
   {
      Node *node = (Node *)FindObjectById(chassis->getControllerId(), OBJECT_NODE);
      if (node != NULL)
      {
         value = node->createNXSLObject(vm);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (!strcmp(attr, "controllerId"))
   {
      value = vm->createValue(chassis->getControllerId());
   }
   else if (!strcmp(attr, "flags"))
   {
      value = vm->createValue(chassis->getFlags());
   }
   else if (!strcmp(attr, "rack"))
   {
      Rack *rack = (Rack *)FindObjectById(chassis->getRackId(), OBJECT_RACK);
      if (rack != NULL)
      {
         value = rack->createNXSLObject(vm);
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (!strcmp(attr, "rackId"))
   {
      value = vm->createValue(chassis->getRackId());
   }
   else if (!strcmp(attr, "rackHeight"))
   {
      value = vm->createValue(chassis->getRackHeight());
   }
   else if (!strcmp(attr, "rackPosition"))
   {
      value = vm->createValue(chassis->getRackPosition());
   }
   return value;
}

/**
 * Cluster::getResourceOwner() method
 */
NXSL_METHOD_DEFINITION(Cluster, getResourceOwner)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   UINT32 ownerId = ((Cluster *)object->getData())->getResourceOwner(argv[0]->getValueAsCString());
   if (ownerId != 0)
   {
      NetObj *object = FindObjectById(ownerId);
      *result = (object != NULL) ? object->createNXSLObject(vm) : vm->createValue();
   }
   else
   {
      *result = vm->createValue();
   }
   return 0;
}

/**
 * NXSL class "Cluster" constructor
 */
NXSL_ClusterClass::NXSL_ClusterClass() : NXSL_DCTargetClass()
{
   setName(_T("Cluster"));

   NXSL_REGISTER_METHOD(Cluster, getResourceOwner, 1);
}

/**
 * NXSL class "Cluster" attributes
 */
NXSL_Value *NXSL_ClusterClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_DCTargetClass::getAttr(object, attr);
   if (value != NULL)
      return value;

   NXSL_VM *vm = object->vm();
   Cluster *cluster = (Cluster *)object->getData();
   if (!strcmp(attr, "nodes"))
   {
      value = vm->createValue(cluster->getNodesForNXSL(vm));
   }
   else if (!strcmp(attr, "zone"))
   {
      if (g_flags & AF_ENABLE_ZONING)
      {
         Zone *zone = FindZoneByUIN(cluster->getZoneUIN());
         if (zone != NULL)
         {
            value = zone->createNXSLObject(vm);
         }
         else
         {
            value = vm->createValue();
         }
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (!strcmp(attr, "zoneUIN"))
   {
      value = vm->createValue(cluster->getZoneUIN());
   }
   return value;
}

/**
 * Container::setAutoBindMode() method
 */
NXSL_METHOD_DEFINITION(Container, setAutoBindMode)
{
   if (!argv[0]->isInteger() || !argv[1]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   ((Container *)object->getData())->setAutoBindMode(argv[0]->getValueAsBoolean(), argv[1]->getValueAsBoolean());
   *result = vm->createValue();
   return 0;
}

/**
 * Container::setAutoBindScript() method
 */
NXSL_METHOD_DEFINITION(Container, setAutoBindScript)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   ((Container *)object->getData())->setAutoBindFilter(argv[0]->getValueAsCString());
   *result = vm->createValue();
   return 0;
}

/**
 * NXSL class "Container" constructor
 */
NXSL_ContainerClass::NXSL_ContainerClass() : NXSL_NetObjClass()
{
   setName(_T("Container"));

   NXSL_REGISTER_METHOD(Container, setAutoBindMode, 2);
   NXSL_REGISTER_METHOD(Container, setAutoBindScript, 1);
}

/**
 * NXSL class "Cluster" attributes
 */
NXSL_Value *NXSL_ContainerClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_NetObjClass::getAttr(object, attr);
   if (value != NULL)
      return value;

   NXSL_VM *vm = object->vm();
   Container *container = (Container *)object->getData();
   if (!strcmp(attr, "autoBindScript"))
   {
      const TCHAR *script = container->getAutoBindScriptSource();
      value = vm->createValue(CHECK_NULL_EX(script));
   }
   else if (!strcmp(attr, "isAutoBindEnabled"))
   {
      value = vm->createValue(container->isAutoBindEnabled() ? 1 : 0);
   }
   else if (!strcmp(attr, "isAutoUnbindEnabled"))
   {
      value = vm->createValue(container->isAutoUnbindEnabled() ? 1 : 0);
   }
   return value;
}

/**
 * Event::setMessage() method
 */
NXSL_METHOD_DEFINITION(Event, setMessage)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   Event *event = static_cast<Event*>(object->getData());
   event->setMessage(argv[0]->getValueAsCString());
   *result = vm->createValue();
   return 0;
}

/**
 * Event::setSeverity() method
 */
NXSL_METHOD_DEFINITION(Event, setSeverity)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_STRING;

   int s = argv[0]->getValueAsInt32();
   if ((s >= SEVERITY_NORMAL) && (s <= SEVERITY_CRITICAL))
   {
      Event *event = static_cast<Event*>(object->getData());
      event->setSeverity(s);
   }
   *result = vm->createValue();
   return 0;
}

/**
 * Event::addParameter() method
 */
NXSL_METHOD_DEFINITION(Event, addParameter)
{
   if ((argc < 1) || (argc > 2))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString() || ((argc == 2) && !argv[1]->isString()))
      return NXSL_ERR_NOT_STRING;

   Event *event = static_cast<Event*>(object->getData());
   if (argc == 1)
      event->addParameter(_T(""), argv[0]->getValueAsCString());
   else
      event->addParameter(argv[0]->getValueAsCString(), argv[1]->getValueAsCString());
   *result = vm->createValue();
   return 0;
}

/**
 * Event::addTag() method
 */
NXSL_METHOD_DEFINITION(Event, addTag)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   Event *event = static_cast<Event*>(object->getData());
   event->addTag(argv[0]->getValueAsCString());
   *result = vm->createValue();
   return 0;
}

/**
 * Event::correlateTo() method
 */
NXSL_METHOD_DEFINITION(Event, correlateTo)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   Event *event = static_cast<Event*>(object->getData());
   event->setRootId(argv[0]->getValueAsUInt64());
   *result = vm->createValue();
   return 0;
}

/**
 * Event::expandString() method
 */
NXSL_METHOD_DEFINITION(Event, expandString)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   Event *event = static_cast<Event*>(object->getData());
   *result = vm->createValue(event->expandText(argv[0]->getValueAsCString()));
   return 0;
}

/**
 * Event::hasTag() method
 */
NXSL_METHOD_DEFINITION(Event, hasTag)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   Event *event = static_cast<Event*>(object->getData());
   *result = vm->createValue(event->hasTag(argv[0]->getValueAsCString()));
   return 0;
}

/**
 * Event::removeTag() method
 */
NXSL_METHOD_DEFINITION(Event, removeTag)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   Event *event = static_cast<Event*>(object->getData());
   event->removeTag(argv[0]->getValueAsCString());
   *result = vm->createValue();
   return 0;
}

/**
 * Event::toJson() method
 */
NXSL_METHOD_DEFINITION(Event, toJson)
{
   Event *event = static_cast<Event*>(object->getData());
   json_t *json = event->toJson();
   char *text = json_dumps(json, JSON_INDENT(3) | JSON_EMBED);
   *result = vm->createValue(text);
   MemFree(text);
   json_decref(json);
   return 0;
}

/**
 * NXSL class Event: constructor
 */
NXSL_EventClass::NXSL_EventClass() : NXSL_Class()
{
   setName(_T("Event"));

   NXSL_REGISTER_METHOD(Event, addParameter, -1);
   NXSL_REGISTER_METHOD(Event, addTag, 1);
   NXSL_REGISTER_METHOD(Event, correlateTo, 1);
   NXSL_REGISTER_METHOD(Event, expandString, 1);
   NXSL_REGISTER_METHOD(Event, hasTag, 1);
   NXSL_REGISTER_METHOD(Event, removeTag, 1);
   NXSL_REGISTER_METHOD(Event, setMessage, 1);
   NXSL_REGISTER_METHOD(Event, setSeverity, 1);
   NXSL_REGISTER_METHOD(Event, toJson, 0);
}

/**
 * Destructor
 */
void NXSL_EventClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<Event*>(object->getData());
}

/**
 * NXSL class Event: get attribute
 */
NXSL_Value *NXSL_EventClass::getAttr(NXSL_Object *pObject, const char *attr)
{
   NXSL_Value *value = NULL;

   NXSL_VM *vm = pObject->vm();
   const Event *event = static_cast<Event*>(pObject->getData());
   if (!strcmp(attr, "code"))
   {
      value = vm->createValue(event->getCode());
   }
   else if (!strcmp(attr, "customMessage"))
   {
      value = vm->createValue(event->getCustomMessage());
   }
   else if (!strcmp(attr, "dci"))
   {
      UINT32 dciId = event->getDciId();
      if (dciId != 0)
      {
         NetObj *object = FindObjectById(event->getSourceId());
         if ((object != NULL) && object->isDataCollectionTarget())
         {
            shared_ptr<DCObject> dci = static_cast<DataCollectionTarget*>(object)->getDCObjectById(dciId, 0, true);
            if (dci != NULL)
            {
               value = dci->createNXSLObject(vm);
            }
            else
            {
               value = vm->createValue();
            }
         }
         else
         {
            value = vm->createValue();
         }
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (!strcmp(attr, "dciId"))
   {
      value = vm->createValue(event->getDciId());
   }
   else if (!strcmp(attr, "id"))
   {
      value = vm->createValue(event->getId());
   }
   else if (!strcmp(attr, "message"))
   {
      value = vm->createValue(event->getMessage());
   }
   else if (!strcmp(attr, "name"))
   {
		value = vm->createValue(event->getName());
   }
   else if (!strcmp(attr, "origin"))
   {
      value = vm->createValue(static_cast<INT32>(event->getOrigin()));
   }
   else if (!strcmp(attr, "originTimestamp"))
   {
      value = vm->createValue(static_cast<INT64>(event->getOriginTimestamp()));
   }
   else if (!strcmp(attr, "parameters"))
   {
      NXSL_Array *array = new NXSL_Array(vm);
      for(int i = 0; i < event->getParametersCount(); i++)
         array->set(i + 1, vm->createValue(event->getParameter(i)));
      value = vm->createValue(array);
   }
   else if (!strcmp(attr, "parameterNames"))
   {
      NXSL_Array *array = new NXSL_Array(vm);
      for(int i = 0; i < event->getParametersCount(); i++)
         array->set(i + 1, vm->createValue(event->getParameterName(i)));
      value = vm->createValue(array);
   }
   else if (!strcmp(attr, "rootId"))
   {
      value = vm->createValue(event->getRootId());
   }
   else if (!strcmp(attr, "severity"))
   {
      value = vm->createValue(event->getSeverity());
   }
   else if (!strcmp(attr, "source"))
   {
      NetObj *object = FindObjectById(event->getSourceId());
      value = (object != NULL) ? object->createNXSLObject(vm) : vm->createValue();
   }
   else if (!strcmp(attr, "sourceId"))
   {
      value = vm->createValue(event->getSourceId());
   }
   else if (!strcmp(attr, "tagList"))
   {
      value = vm->createValue(event->getTagsAsList());
   }
   else if (!strcmp(attr, "tags"))
   {
      StringList *tags = String(event->getTagsAsList()).split(_T(","));
      value = vm->createValue(new NXSL_Array(vm, tags));
      delete tags;
   }
   else if (!strcmp(attr, "timestamp"))
   {
      value = vm->createValue(static_cast<INT64>(event->getTimestamp()));
   }
   else
   {
      if (attr[0] == _T('$'))
      {
         // Try to find parameter with given index
         char *eptr;
         int index = strtol(&attr[1], &eptr, 10);
         if ((index > 0) && (*eptr == 0))
         {
            const TCHAR *s = event->getParameter(index - 1);
            if (s != NULL)
            {
               value = vm->createValue(s);
            }
         }
      }

      // Try to find named parameter with given name
      if (value == NULL)
      {
#ifdef UNICODE
         WCHAR wattr[MAX_IDENTIFIER_LENGTH];
         MultiByteToWideChar(CP_UTF8, 0, attr, -1, wattr, MAX_IDENTIFIER_LENGTH);
         wattr[MAX_IDENTIFIER_LENGTH - 1] = 0;
         const TCHAR *s = event->getNamedParameter(wattr);
#else
         const TCHAR *s = event->getNamedParameter(attr);
#endif
         if (s != NULL)
         {
            value = vm->createValue(s);
         }
      }
   }
   return value;
}

/**
 * NXSL class Event: set attribute
 */
bool NXSL_EventClass::setAttr(NXSL_Object *object, const char *attr, NXSL_Value *value)
{
   Event *event = static_cast<Event*>(object->getData());
   if (!strcmp(attr, "customMessage"))
   {
      if (value->isString())
      {
         event->setCustomMessage(value->getValueAsCString());
      }
      else
      {
         event->setCustomMessage(NULL);
      }
   }
   else if (!strcmp(attr, "message"))
   {
      if (value->isString())
      {
         event->setMessage(value->getValueAsCString());
      }
   }
   else if (!strcmp(attr, "severity"))
   {
      int s = value->getValueAsInt32();
      if ((s >= SEVERITY_NORMAL) && (s <= SEVERITY_CRITICAL))
      {
         event->setSeverity(s);
      }
   }
   else
   {
      bool success = false;
      if (attr[0] == _T('$'))
      {
         // Try to find parameter with given index
         char *eptr;
         int index = strtol(&attr[1], &eptr, 10);
         if ((index > 0) && (*eptr == 0))
         {
            const TCHAR *name = event->getParameterName(index - 1);
            event->setParameter(index - 1, CHECK_NULL_EX(name), value->getValueAsCString());
            success = true;
         }
      }
      if (!success)
      {
#ifdef UNICODE
         WCHAR wattr[MAX_IDENTIFIER_LENGTH];
         MultiByteToWideChar(CP_UTF8, 0, attr, -1, wattr, MAX_IDENTIFIER_LENGTH);
         wattr[MAX_IDENTIFIER_LENGTH - 1] = 0;
         event->setNamedParameter(wattr, value->getValueAsCString());
#else
         event->setNamedParameter(attr, value->getValueAsCString());
#endif
      }
   }
   return true;
}

/**
 * Alarm::acknowledge() method
 */
NXSL_METHOD_DEFINITION(Alarm, acknowledge)
{
   if (argc > 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   Alarm *alarm = static_cast<Alarm*>(object->getData());
   *result = vm->createValue(AckAlarmById(alarm->getAlarmId(), NULL, false, 0, (argc == 1) ? argv[0]->getValueAsBoolean() : false));
   return 0;
}

/**
 * Alarm::resolve() method
 */
NXSL_METHOD_DEFINITION(Alarm, resolve)
{
   if (argc > 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   Alarm *alarm = static_cast<Alarm*>(object->getData());
   *result = vm->createValue(ResolveAlarmById(alarm->getAlarmId(), NULL, false, (argc == 1) ? argv[0]->getValueAsBoolean() : false));
   return 0;
}

/**
 * Alarm::terminate() method
 */
NXSL_METHOD_DEFINITION(Alarm, terminate)
{
   if (argc > 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   Alarm *alarm = static_cast<Alarm*>(object->getData());
   *result = vm->createValue(ResolveAlarmById(alarm->getAlarmId(), NULL, true, (argc == 1) ? argv[0]->getValueAsBoolean() : false));
   return 0;
}

/**
 * Alarm::addComment() method
 */
NXSL_METHOD_DEFINITION(Alarm, addComment)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   bool syncWithHelpdesk = true;
   if (argc > 1)
   {
      if(!argv[1]->isInteger())
         return NXSL_ERR_NOT_INTEGER;
      else
         syncWithHelpdesk = argv[1]->getValueAsBoolean();
   }

   Alarm *alarm = static_cast<Alarm*>(object->getData());
   UINT32 id = 0;
   UINT32 rcc = UpdateAlarmComment(alarm->getAlarmId(), &id, argv[0]->getValueAsCString(), 0, syncWithHelpdesk);
   if(rcc != RCC_SUCCESS)
      return NXSL_ERR_INTERNAL;

   *result = vm->createValue(id);
   return 0;
}

/**
 * Alarm::getComments() method
 */
NXSL_METHOD_DEFINITION(Alarm, getComments)
{
   NXSL_Array *array = new NXSL_Array(vm);
   ObjectArray<AlarmComment> *alarmComments = GetAlarmComments(((Alarm *)object->getData())->getAlarmId());
   for(int i = 0; i < alarmComments->size(); i++)
   {
      array->append(vm->createValue(new NXSL_Object(vm, &g_nxslAlarmCommentClass, alarmComments->get(i))));
   }
   delete alarmComments;
   *result = vm->createValue(array);
   return 0;
}

/**
 * NXSL class Alarm: constructor
 */
NXSL_AlarmClass::NXSL_AlarmClass() : NXSL_Class()
{
   setName(_T("Alarm"));

   NXSL_REGISTER_METHOD(Alarm, acknowledge, -1);
   NXSL_REGISTER_METHOD(Alarm, resolve, -1);
   NXSL_REGISTER_METHOD(Alarm, terminate, -1);
   NXSL_REGISTER_METHOD(Alarm, addComment, -1);
   NXSL_REGISTER_METHOD(Alarm, getComments, 0);
}

/**
 * NXSL object destructor
 */
void NXSL_AlarmClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<Alarm*>(object->getData());
}

/**
 * NXSL class Alarm: get attribute
 */
NXSL_Value *NXSL_AlarmClass::getAttr(NXSL_Object *pObject, const char *attr)
{
   NXSL_VM *vm = pObject->vm();
   NXSL_Value *value = NULL;
   Alarm *alarm = (Alarm *)pObject->getData();

   if (!strcmp(attr, "ackBy"))
   {
      value = vm->createValue(alarm->getAckByUser());
   }
   else if (!strcmp(attr, "creationTime"))
   {
      value = vm->createValue((INT64)alarm->getCreationTime());
   }
   else if (!strcmp(attr, "dciId"))
   {
      value = vm->createValue(alarm->getDciId());
   }
   else if (!strcmp(attr, "eventCode"))
   {
      value = vm->createValue(alarm->getSourceEventCode());
   }
   else if (!strcmp(attr, "eventId"))
   {
      value = vm->createValue(alarm->getSourceEventId());
   }
   else if (!strcmp(attr, "eventTagList"))
   {
      value = vm->createValue(alarm->getEventTags());
   }
   else if (!strcmp(attr, "helpdeskReference"))
   {
      value = vm->createValue(alarm->getHelpDeskRef());
   }
   else if (!strcmp(attr, "helpdeskState"))
   {
      value = vm->createValue(alarm->getHelpDeskState());
   }
   else if (!strcmp(attr, "id"))
   {
      value = vm->createValue(alarm->getAlarmId());
   }
   else if (!strcmp(attr, "impact"))
   {
      value = vm->createValue(alarm->getImpact());
   }
   else if (!strcmp(attr, "key"))
   {
      value = vm->createValue(alarm->getKey());
   }
   else if (!strcmp(attr, "lastChangeTime"))
   {
      value = vm->createValue((INT64)alarm->getLastChangeTime());
   }
   else if (!strcmp(attr, "message"))
   {
      value = vm->createValue(alarm->getMessage());
   }
   else if (!strcmp(attr, "originalSeverity"))
   {
      value = vm->createValue(alarm->getOriginalSeverity());
   }
   else if (!strcmp(attr, "parentId"))
   {
      value = vm->createValue(alarm->getParentAlarmId());
   }
   else if (!strcmp(attr, "repeatCount"))
   {
      value = vm->createValue(alarm->getRepeatCount());
   }
   else if (!strcmp(attr, "resolvedBy"))
   {
      value = vm->createValue(alarm->getResolvedByUser());
   }
   else if (!strcmp(attr, "rcaScriptName"))
   {
      value = vm->createValue(alarm->getRcaScriptName());
   }
   else if (!strcmp(attr, "ruleGuid"))
   {
      TCHAR buffer[64];
      value = vm->createValue(alarm->getRule().toString(buffer));
   }
   else if (!strcmp(attr, "severity"))
   {
      value = vm->createValue(alarm->getCurrentSeverity());
   }
   else if (!strcmp(attr, "sourceObject"))
   {
      value = vm->createValue(alarm->getSourceObject());
   }
   else if (!strcmp(attr, "state"))
   {
      value = vm->createValue(alarm->getState());
   }
   return value;
}

/**
 * NXSL class Alarm: constructor
 */
NXSL_AlarmCommentClass::NXSL_AlarmCommentClass() : NXSL_Class()
{
   setName(_T("AlarmComment"));
}

/**
 * NXSL object destructor
 */
void NXSL_AlarmCommentClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<AlarmComment*>(object->getData());
}

/**
 * NXSL class Alarm: get attribute
 */
NXSL_Value *NXSL_AlarmCommentClass::getAttr(NXSL_Object *pObject, const char *attr)
{
   NXSL_VM *vm = pObject->vm();
   NXSL_Value *value = NULL;
   AlarmComment *alarmComment = (AlarmComment *)pObject->getData();

   if (!strcmp(attr, "id"))
   {
      value = vm->createValue(alarmComment->getId());
   }
   else if (!strcmp(attr, "changeTime"))
   {
      value = vm->createValue((INT64)alarmComment->getChangeTime());
   }
   else if (!strcmp(attr, "userId"))
   {
      value = vm->createValue(alarmComment->getUserId());
   }
   else if (!strcmp(attr, "text"))
   {
      value = vm->createValue(alarmComment->getText());
   }
   return value;
}

/**
 * DCI::forcePoll() method
 */
NXSL_METHOD_DEFINITION(DCI, forcePoll)
{
   DCObjectInfo *dci = static_cast<DCObjectInfo*>(object->getData());
   NetObj *dcTarget = FindObjectById(dci->getOwnerId());
   if ((dcTarget != NULL) && dcTarget->isDataCollectionTarget())
   {
      shared_ptr<DCObject> dcObject = static_cast<DataCollectionTarget*>(dcTarget)->getDCObjectById(dci->getId(), 0, true);
      if (dcObject != NULL)
      {
         dcObject->requestForcePoll(NULL);
      }
   }
   *result = vm->createValue();
   return 0;
}

/**
 * Implementation of "DCI" class: constructor
 */
NXSL_DciClass::NXSL_DciClass() : NXSL_Class()
{
   setName(_T("DCI"));

   NXSL_REGISTER_METHOD(DCI, forcePoll, 0);
}

/**
 * Object destructor
 */
void NXSL_DciClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<DCObjectInfo*>(object->getData());
}

/**
 * Implementation of "DCI" class: get attribute
 */
NXSL_Value *NXSL_DciClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_VM *vm = object->vm();
   DCObjectInfo *dci = static_cast<DCObjectInfo*>(object->getData());
   NXSL_Value *value = NULL;
   if (!strcmp(attr, "activeThresholdSeverity"))
   {
      value = vm->createValue(dci->getThresholdSeverity());
   }
   else if (!strcmp(attr, "comments"))
   {
		value = vm->createValue(dci->getComments());
   }
   else if (!strcmp(attr, "dataType") && (dci->getType() == DCO_TYPE_ITEM))
   {
		value = vm->createValue(dci->getDataType());
   }
   else if (!strcmp(attr, "description"))
   {
		value = vm->createValue(dci->getDescription());
   }
   else if (!strcmp(attr, "errorCount"))
   {
		value = vm->createValue(dci->getErrorCount());
   }
   else if (!strcmp(attr, "hasActiveThreshold"))
   {
      value = vm->createValue(dci->hasActiveThreshold() ? 1 : 0);
   }
   else if (!strcmp(attr, "id"))
   {
		value = vm->createValue(dci->getId());
   }
   else if (!strcmp(attr, "instance"))
   {
		value = vm->createValue(dci->getInstance());
   }
   else if (!strcmp(attr, "instanceData"))
   {
		value = vm->createValue(dci->getInstanceData());
   }
   else if (!strcmp(attr, "lastPollTime"))
   {
		value = vm->createValue((INT64)dci->getLastPollTime());
   }
   else if (!strcmp(attr, "name"))
   {
		value = vm->createValue(dci->getName());
   }
   else if (!strcmp(attr, "origin"))
   {
		value = vm->createValue((LONG)dci->getOrigin());
   }
   else if (!strcmp(attr, "pollingInterval"))
   {
      value = vm->createValue(dci->getPollingInterval());
   }
   else if (!strcmp(attr, "relatedObject"))
   {
      if (dci->getRelatedObject() != 0)
      {
         NetObj *object = FindObjectById(dci->getRelatedObject());
         value = (object != NULL) ? object->createNXSLObject(vm) : vm->createValue();
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (!strcmp(attr, "status"))
   {
		value = vm->createValue((LONG)dci->getStatus());
   }
   else if (!strcmp(attr, "systemTag"))
   {
		value = vm->createValue(dci->getSystemTag());
   }
   else if (!strcmp(attr, "template"))
   {
      if (dci->getTemplateId() != 0)
      {
         NetObj *object = FindObjectById(dci->getTemplateId());
         value = (object != NULL) ? object->createNXSLObject(vm) : vm->createValue();
      }
      else
      {
         value = vm->createValue();
      }
   }
   else if (!strcmp(attr, "templateId"))
   {
      value = vm->createValue(dci->getTemplateId());
   }
   else if (!strcmp(attr, "templateItemId"))
   {
      value = vm->createValue(dci->getTemplateItemId());
   }
   else if (!strcmp(attr, "type"))
   {
		value = vm->createValue((LONG)dci->getType());
   }
   return value;
}

/**
 * Implementation of "Sensor" class: constructor
 */
NXSL_SensorClass::NXSL_SensorClass() : NXSL_DCTargetClass()
{
   setName(_T("Sensor"));
}

/**
 * Implementation of "Sensor" class: get attribute
 */
NXSL_Value *NXSL_SensorClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_DCTargetClass::getAttr(object, attr);
   if (value != NULL)
      return value;

   NXSL_VM *vm = object->vm();
   Sensor *s = (Sensor *)object->getData();
   if (!strcmp(attr, "description"))
   {
      value = vm->createValue(s->getDescription());
   }
   else if (!strcmp(attr, "frameCount"))
   {
      value = vm->createValue(s->getFrameCount());
   }
   else if (!strcmp(attr, "lastContact"))
   {
      value = vm->createValue((UINT32)s->getLastContact());
   }
   else if (!strcmp(attr, "metaType"))
   {
      value = vm->createValue(s->getMetaType());
   }
   else if (!strcmp(attr, "protocol"))
   {
      value = vm->createValue(s->getCommProtocol());
   }
   else if (!strcmp(attr, "serial"))
   {
      value = vm->createValue(s->getSerialNumber());
   }
   else if (!strcmp(attr, "type"))
   {
      value = vm->createValue(s->getSensorClass());
   }
   else if (!strcmp(attr, "vendor"))
   {
      value = vm->createValue(s->getVendor());
   }

   return value;
}

/**
 * SNMPTransport::get() method
 */
NXSL_METHOD_DEFINITION(SNMPTransport, get)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   SNMP_Transport *transport = static_cast<SNMP_Transport*>(object->getData());

   // Create PDU and send request
   UINT32 oid[MAX_OID_LEN];
   size_t olen = SNMPParseOID(argv[0]->getValueAsCString(), oid, MAX_OID_LEN);
   if (olen == 0)
      return NXSL_ERR_BAD_CONDITION;

   SNMP_PDU *pdu = new SNMP_PDU(SNMP_GET_REQUEST, SnmpNewRequestId(), transport->getSnmpVersion());
   pdu->bindVariable(new SNMP_Variable(oid, olen));

   SNMP_PDU *rspPDU;
   UINT32 rc = transport->doRequest(pdu, &rspPDU, SnmpGetDefaultTimeout(), 3);
   if (rc == SNMP_ERR_SUCCESS)
   {
      if ((rspPDU->getNumVariables() > 0) && (rspPDU->getErrorCode() == SNMP_PDU_ERR_SUCCESS))
      {
         SNMP_Variable *pVar = rspPDU->getVariable(0);
         *result = vm->createValue(new NXSL_Object(vm, &g_nxslSnmpVarBindClass, pVar));
         rspPDU->unlinkVariables();
      }
      else
      {
         *result = vm->createValue();
      }
      delete rspPDU;
   }
   else
   {
      *result = vm->createValue();
   }
   delete pdu;
   return 0;
}

/**
 * SNMPTransport::getValue() method
 */
NXSL_METHOD_DEFINITION(SNMPTransport, getValue)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   SNMP_Transport *transport = static_cast<SNMP_Transport*>(object->getData());

   TCHAR buffer[4096];
   if (SnmpGetEx(transport, argv[0]->getValueAsCString(), NULL, 0, buffer, sizeof(buffer), SG_STRING_RESULT, NULL) == SNMP_ERR_SUCCESS)
   {
      *result = vm->createValue(buffer);
   }
   else
   {
      *result = vm->createValue();
   }

   return 0;
}

/**
 * SNMPTransport::set() method
 */
NXSL_METHOD_DEFINITION(SNMPTransport, set)
{
   if (argc < 2 || argc > 3)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;
   if (!argv[0]->isString() || !argv[1]->isString() || ((argc == 3) && !argv[2]->isString()))
      return NXSL_ERR_NOT_STRING;

   SNMP_Transport *transport = static_cast<SNMP_Transport*>(object->getData());

   SNMP_PDU *request = new SNMP_PDU(SNMP_SET_REQUEST, SnmpNewRequestId(), transport->getSnmpVersion());
   SNMP_PDU *response = NULL;
   bool success = false;

   if (SNMPIsCorrectOID(argv[0]->getValueAsCString()))
   {
      SNMP_Variable *var = new SNMP_Variable(argv[0]->getValueAsCString());
      if (argc == 2)
      {
         var->setValueFromString(ASN_OCTET_STRING, argv[1]->getValueAsCString());
      }
      else
      {
         UINT32 dataType = SNMPResolveDataType(argv[2]->getValueAsCString());
         if (dataType == ASN_NULL)
         {
            nxlog_debug_tag(_T("snmp.nxsl"), 6, _T("SNMPTransport::set: failed to resolve data type '%s', assume string"),
               argv[2]->getValueAsCString());
            dataType = ASN_OCTET_STRING;
         }
         var->setValueFromString(dataType, argv[1]->getValueAsCString());
      }
      request->bindVariable(var);
   }
   else
   {
      nxlog_debug_tag(_T("snmp.nxsl"), 6, _T("SNMPTransport::set: Invalid OID: %s"), argv[0]->getValueAsCString());
      goto finish;
   }

   // Send request and process response
   UINT32 snmpResult;
   if ((snmpResult = transport->doRequest(request, &response, SnmpGetDefaultTimeout(), 3)) == SNMP_ERR_SUCCESS)
   {
      if (response->getErrorCode() != 0)
      {
         nxlog_debug_tag(_T("snmp.nxsl"), 6, _T("SNMPTransport::set: operation failed (error code %d)"), response->getErrorCode());
         goto finish;
      }
      else
      {
         nxlog_debug_tag(_T("snmp.nxsl"), 6, _T("SNMPTransport::set: success"));
         success = true;
      }
      delete response;
   }
   else
   {
      nxlog_debug_tag(_T("snmp.nxsl"), 6, _T("SNMPTransport::set: %s"), SNMPGetErrorText(snmpResult));
   }

finish:
   delete request;
   *result = vm->createValue(success);
   return 0;
}

/**
 * SNMP walk callback
 */
static UINT32 WalkCallback(SNMP_Variable *var, SNMP_Transport *transport, void *context)
{
   NXSL_VM *vm = static_cast<NXSL_VM*>(static_cast<NXSL_Array*>(context)->vm());
   static_cast<NXSL_Array*>(context)->append(vm->createValue(new NXSL_Object(vm, &g_nxslSnmpVarBindClass, new SNMP_Variable(var))));
   return SNMP_ERR_SUCCESS;
}

/**
 * SNMPTransport::walk() method
 */
NXSL_METHOD_DEFINITION(SNMPTransport, walk)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   SNMP_Transport *transport = static_cast<SNMP_Transport*>(object->getData());

   NXSL_Array *varbinds = new NXSL_Array(vm);
   if (SnmpWalk(transport, argv[0]->getValueAsCString(), WalkCallback, varbinds) == SNMP_ERR_SUCCESS)
   {
      *result = vm->createValue(varbinds);
   }
   else
   {
      *result = vm->createValue();
      delete varbinds;
   }
   return 0;
}

/**
 * Implementation of "SNMP_Transport" class: constructor
 */
NXSL_SNMPTransportClass::NXSL_SNMPTransportClass() : NXSL_Class()
{
	setName(_T("SNMPTransport"));

   NXSL_REGISTER_METHOD(SNMPTransport, get, 1);
   NXSL_REGISTER_METHOD(SNMPTransport, getValue, 1);
   NXSL_REGISTER_METHOD(SNMPTransport, set, -1);
   NXSL_REGISTER_METHOD(SNMPTransport, walk, 1);
}

/**
 * Implementation of "SNMP_Transport" class: get attribute
 */
NXSL_Value *NXSL_SNMPTransportClass::getAttr(NXSL_Object *object, const char *attr)
{
	NXSL_Value *value = NULL;
	SNMP_Transport *t = static_cast<SNMP_Transport*>(object->getData());

	if (!strcmp(attr, "snmpVersion"))
	{
		const TCHAR *versions[] = { _T("1"), _T("2c"), _T("3") };
		value = object->vm()->createValue((const TCHAR *)versions[t->getSnmpVersion()]);
	}

	return value;
}

/**
 * Implementation of "SNMP_Transport" class: NXSL object destructor
 */
void NXSL_SNMPTransportClass::onObjectDelete(NXSL_Object *object)
{
	delete (SNMP_Transport *)object->getData();
}

/**
 * NXSL class SNMP_VarBind: constructor
 */
NXSL_SNMPVarBindClass::NXSL_SNMPVarBindClass() : NXSL_Class()
{
	setName(_T("SNMP_VarBind"));
}

/**
 * NXSL class SNMP_VarBind: get attribute
 */
NXSL_Value *NXSL_SNMPVarBindClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_VM *vm = object->vm();
	NXSL_Value *value = NULL;
	SNMP_Variable *t = (SNMP_Variable *)object->getData();
	if (!strcmp(attr, "type"))
	{
		value = vm->createValue((UINT32)t->getType());
	}
	else if (!strcmp(attr, "name"))
	{
		value = vm->createValue(t->getName().toString());
	}
	else if (!strcmp(attr, "value"))
	{
   	TCHAR strValue[1024];
		value = vm->createValue(t->getValueAsString(strValue, 1024));
	}
	else if (!strcmp(attr, "printableValue"))
	{
   	TCHAR strValue[1024];
		bool convToHex = true;
		t->getValueAsPrintableString(strValue, 1024, &convToHex);
		value = vm->createValue(strValue);
	}
	else if (!strcmp(attr, "valueAsIp"))
	{
   	TCHAR strValue[128];
		t->getValueAsIPAddr(strValue);
		value = vm->createValue(strValue);
	}
	else if (!strcmp(attr, "valueAsMac"))
	{
   	TCHAR strValue[128];
		value = vm->createValue(t->getValueAsMACAddr().toString(strValue));
	}

	return value;
}

/**
 * NXSL class SNMP_VarBind: NXSL object desctructor
 */
void NXSL_SNMPVarBindClass::onObjectDelete(NXSL_Object *object)
{
	delete (SNMP_Variable *)object->getData();
}

/**
 * NXSL class UserDBObjectClass: constructor
 */
NXSL_UserDBObjectClass::NXSL_UserDBObjectClass() : NXSL_Class()
{
   setName(_T("UserDBObject"));
}

/**
 * NXSL class UserDBObjectClass: get attribute
 */
NXSL_Value *NXSL_UserDBObjectClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_VM *vm = object->vm();
   NXSL_Value *value = NULL;
   UserDatabaseObject *dbObject = static_cast<UserDatabaseObject *>(object->getData());

   if (!strcmp(attr, "description"))
   {
      value = vm->createValue(dbObject->getDescription());
   }
   else if (!strcmp(attr, "flags"))
   {
      value = vm->createValue(dbObject->getFlags());
   }
   else if (!strcmp(attr, "guid"))
   {
      TCHAR buffer[64];
      value = vm->createValue(dbObject->getGuidAsText(buffer));
   }
   else if (!strcmp(attr, "id"))
   {
      value = vm->createValue(dbObject->getId());
   }
   else if (!strcmp(attr, "isDeleted"))
   {
      value = vm->createValue(dbObject->isDeleted());
   }
   else if (!strcmp(attr, "isDisabled"))
   {
      value = vm->createValue(dbObject->isDisabled());
   }
   else if (!strcmp(attr, "isGroup"))
   {
      value = vm->createValue(dbObject->isGroup());
   }
   else if (!strcmp(attr, "isModified"))
   {
      value = vm->createValue(dbObject->isModified());
   }
   else if (!strcmp(attr, "isLDAPUser"))
   {
      value = vm->createValue(dbObject->isLDAPUser());
   }
   else if (!strcmp(attr, "ldapDomain"))
   {
      value = vm->createValue(dbObject->getDn());
   }
   else if (!strcmp(attr, "ldapId"))
   {
      value = vm->createValue(dbObject->getLdapId());
   }
   else if (!strcmp(attr, "systemRights"))
   {
      value = vm->createValue(dbObject->getSystemRights());
   }

   return value;
}

/**
 * NXSL class UserClass: constructor
 */
NXSL_UserClass::NXSL_UserClass() : NXSL_UserDBObjectClass()
{
   setName(_T("User"));
}

/**
 * NXSL class UserDBObjectClass: get attribute
 */
NXSL_Value *NXSL_UserClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_UserDBObjectClass::getAttr(object, attr);
   if (value != NULL)
      return value;

   NXSL_VM *vm = object->vm();
   User *user = static_cast<User *>(object->getData());
   if (!strcmp(attr, "authMethod"))
   {
      value = vm->createValue(user->getAuthMethod());
   }
   else if (!strcmp(attr, "certMappingData"))
   {
      value = vm->createValue(user->getCertMappingData());
   }
   else if (!strcmp(attr, "certMappingMethod"))
   {
      value = vm->createValue(user->getCertMappingMethod());
   }
   else if (!strcmp(attr, "disabledUntil"))
   {
      value = vm->createValue(static_cast<UINT32>(user->getReEnableTime()));
   }
   else if (!strcmp(attr, "fullName"))
   {
      value = vm->createValue(user->getFullName());
   }
   else if (!strcmp(attr, "graceLogins"))
   {
      value = vm->createValue(user->getGraceLogins());
   }
   else if (!strcmp(attr, "lastLogin"))
   {
      value = vm->createValue(static_cast<UINT32>(user->getLastLoginTime()));
   }
   else if (!strcmp(attr, "xmppId"))
   {
      value = vm->createValue(user->getXmppId());
   }

   return value;
}

/**
 * NXSL class User: NXSL object destructor
 */
void NXSL_UserClass::onObjectDelete(NXSL_Object *object)
{
   delete (User *)object->getData();
}

/**
 * NXSL class UserGroupClass: constructor
 */
NXSL_UserGroupClass::NXSL_UserGroupClass() : NXSL_UserDBObjectClass()
{
   setName(_T("UserGroup"));
}

/**
 * NXSL class UserDBObjectClass: get attribute
 */
NXSL_Value *NXSL_UserGroupClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_Value *value = NXSL_UserDBObjectClass::getAttr(object, attr);
   if (value != NULL)
      return value;

   NXSL_VM *vm = object->vm();
   Group *group = static_cast<Group*>(object->getData());
   if (!strcmp(attr, "memberCount"))
   {
      value = vm->createValue(group->getMemberCount());
   }
   else if (!strcmp(attr, "members"))
   {
      UINT32 *members = NULL;
      int count = group->getMembers(&members);
      IntegerArray<UINT32> memberArray;
      for(int i = 0; i < count; i++)
         memberArray.add(members[i]);

      NXSL_Array *array = new NXSL_Array(vm);
      ObjectArray<UserDatabaseObject> *userDB = FindUserDBObjects(&memberArray);
      userDB->setOwner(Ownership::False);
      for(int i = 0; i < userDB->size(); i++)
      {
         if (userDB->get(i)->isGroup())
            array->append(vm->createValue(new NXSL_Object(vm, &g_nxslUserGroupClass, userDB->get(i))));
         else
            array->append(vm->createValue(new NXSL_Object(vm, &g_nxslUserClass, userDB->get(i))));
      }
      value = vm->createValue(array);
      delete userDB;
   }

   return value;
}

/**
 * NXSL class UserGroupClass: NXSL object destructor
 */
void NXSL_UserGroupClass::onObjectDelete(NXSL_Object *object)
{
   delete (Group *)object->getData();
}

/**
 * NXSL class NodeDependency: constructor
 */
NXSL_NodeDependencyClass::NXSL_NodeDependencyClass() : NXSL_Class()
{
   setName(_T("NodeDependency"));
}

/**
 * NXSL class NodeDependency: get attribute
 */
NXSL_Value *NXSL_NodeDependencyClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_VM *vm = object->vm();
   NXSL_Value *value = NULL;
   DependentNode *dn = static_cast<DependentNode*>(object->getData());

   if (!strcmp(attr, "id"))
   {
      value = vm->createValue(dn->nodeId);
   }
   else if (!strcmp(attr, "isAgentProxy"))
   {
      value = vm->createValue((dn->dependencyType & NODE_DEP_AGENT_PROXY) != 0);
   }
   else if (!strcmp(attr, "isDataCollectionSource"))
   {
      value = vm->createValue((dn->dependencyType & NODE_DEP_DC_SOURCE) != 0);
   }
   else if (!strcmp(attr, "isICMPProxy"))
   {
      value = vm->createValue((dn->dependencyType & NODE_DEP_ICMP_PROXY) != 0);
   }
   else if (!strcmp(attr, "isSNMPProxy"))
   {
      value = vm->createValue((dn->dependencyType & NODE_DEP_SNMP_PROXY) != 0);
   }
   else if (!strcmp(attr, "type"))
   {
      value = vm->createValue(dn->dependencyType);
   }
   return value;
}

/**
 * NXSL class NodeDependency: NXSL object desctructor
 */
void NXSL_NodeDependencyClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<DependentNode*>(object->getData());
}

/**
 * NXSL "VLAN" class
 */
NXSL_VlanClass::NXSL_VlanClass()
{
   setName(_T("VLAN"));
}

/**
 * NXSL "VLAN" class: get attribute
 */
NXSL_Value *NXSL_VlanClass::getAttr(NXSL_Object *object, const char *attr)
{
   NXSL_VM *vm = object->vm();
   NXSL_Value *value = NULL;
   VlanInfo *vlan = static_cast<VlanInfo*>(object->getData());

   if (!strcmp(attr, "id"))
   {
      value = vm->createValue(vlan->getVlanId());
   }
   else if (!strcmp(attr, "interfaces"))
   {
      NXSL_Array *a = new NXSL_Array(vm);
      Node *node = static_cast<Node*>(FindObjectById(vlan->getNodeId(), OBJECT_NODE));
      if (node != NULL)
      {
         VlanPortInfo *ports = vlan->getPorts();
         for(int i = 0; i < vlan->getNumPorts(); i++)
         {
            Interface *iface = node->findInterfaceByIndex(ports[i].ifIndex);
            if (iface != NULL)
            {
               a->append(iface->createNXSLObject(vm));
            }
         }
      }
      value = vm->createValue(a);
   }
   else if (!strcmp(attr, "name"))
   {
      value = vm->createValue(vlan->getName());
   }
   return value;
}

/**
 * NXSL "VLAN" class: on object delete
 */
void NXSL_VlanClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<VlanInfo*>(object->getData());
}

/**
 * Class objects
 */
NXSL_AccessPointClass g_nxslAccessPointClass;
NXSL_AlarmClass g_nxslAlarmClass;
NXSL_AlarmCommentClass g_nxslAlarmCommentClass;
NXSL_ChassisClass g_nxslChassisClass;
NXSL_ClusterClass g_nxslClusterClass;
NXSL_ContainerClass g_nxslContainerClass;
NXSL_DciClass g_nxslDciClass;
NXSL_EventClass g_nxslEventClass;
NXSL_InterfaceClass g_nxslInterfaceClass;
NXSL_MobileDeviceClass g_nxslMobileDeviceClass;
NXSL_NetObjClass g_nxslNetObjClass;
NXSL_NodeClass g_nxslNodeClass;
NXSL_NodeDependencyClass g_nxslNodeDependencyClass;
NXSL_SensorClass g_nxslSensorClass;
NXSL_SNMPTransportClass g_nxslSnmpTransportClass;
NXSL_SNMPVarBindClass g_nxslSnmpVarBindClass;
NXSL_SubnetClass g_nxslSubnetClass;
NXSL_UserDBObjectClass g_nxslUserDBObjectClass;
NXSL_UserClass g_nxslUserClass;
NXSL_UserGroupClass g_nxslUserGroupClass;
NXSL_VlanClass g_nxslVlanClass;
NXSL_ZoneClass g_nxslZoneClass;
