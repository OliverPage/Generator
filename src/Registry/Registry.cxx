//____________________________________________________________________________
/*
 Copyright (c) 2003-2006, GENIE Neutrino MC Generator Collaboration
 All rights reserved.
 For the licensing terms see $GENIE/USER_LICENSE.

 Author: Costas Andreopoulos <C.V.Andreopoulos@rl.ac.uk>
         CCLRC, Rutherford Appleton Laboratory - May 04, 2004

 For the class documentation see the corresponding header file.

 Important revisions after version 2.0.0 :

*/
//____________________________________________________________________________

#include <cassert>
#include <cstdlib>
#include <sstream>
#include <iomanip>

#include <TH1F.h>
#include <TH2F.h>
#include <TTree.h>
#include <TFolder.h>
#include <TObjString.h>

#include "Messenger/Messenger.h"
#include "Registry/Registry.h"

using namespace genie;

using std::setw;
using std::setfill;
using std::istream;
using std::cout;
using std::endl;
using std::ostringstream;

//____________________________________________________________________________
namespace genie {
 //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 template<class T> void SetRegistryItem(Registry * r, RgKey key, T item)
 {
    string itemtype = typeid(item).name();
    LOG("Registry", pDEBUG)
             << "Set item [" << itemtype << "]: key = "
                                      << key << " --> value = " << item;
    bool lock = r->ItemIsLocked(key); // store, could be true but inhibited
    RegistryItem<T> * reg_item = new RegistryItem<T>(item,lock);
    RgIMapPair config_entry(key, reg_item);
    r->Set(config_entry);
 }
 //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 template<class T> T GetValueOrUseDefault(
                          Registry * r, RgKey key, T def, bool set_def)
 {
  // Return the requested registry item. If it does not exist return
  // the input default value (in this case, if set_def is true it can 
  // override a lock and add the input default as a new registry item)

   T value;
   if(r->Exists(key)) { 
      r->Get(key,value); return value;
   }
   value = def;
   bool was_locked = r->IsLocked();
   if(was_locked) r->UnLock();

   if(set_def) r->Set(key, value);
   if(was_locked) r->Lock();
   return value;
 }
 //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ostream & operator << (ostream & stream, const Registry & registry)
 {
   registry.Print(stream);
   return stream;
 }
 //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
}
//____________________________________________________________________________
Registry::Registry() 
{
  this->Init();
}
//____________________________________________________________________________
Registry::Registry(string name, bool isReadOnly) :
fName             ( name       ),
fIsReadOnly       ( isReadOnly ),
fInhibitItemLocks ( false      )
{

}
//____________________________________________________________________________
Registry::Registry(const Registry & registry) :
fIsReadOnly(false)
{
  this->Copy(registry);
}
//____________________________________________________________________________
Registry::~Registry()
{
  this->Clear(true);
}
//____________________________________________________________________________
void Registry::operator() (RgKey key, RgInt item)
{
  this->Set(key, item);
}
//____________________________________________________________________________
void Registry::operator() (RgKey key, RgBool item)
{
  this->Set(key, item);
}
//____________________________________________________________________________
void Registry::operator() (RgKey key, RgDbl item)
{
  this->Set(key, item);
}
//____________________________________________________________________________
void Registry::operator() (RgKey key, RgCChAr item)
{
  RgStr item2 = RgStr(item); // "const char *" -> "string"
  this->Set(key,item2);
}
//____________________________________________________________________________
void Registry::operator() (RgKey key, RgStr item)
{
  this->Set(key, item);
}
//____________________________________________________________________________
Registry & Registry::operator = (const Registry & reg)
{
  this->Copy(reg);
  return (*this);
}
//____________________________________________________________________________
Registry & Registry::operator += (const Registry & reg)
{
  this->Append(reg);
  return (*this);
}
//____________________________________________________________________________
void Registry::Lock(void)
{
  fIsReadOnly = true;
}
//____________________________________________________________________________
void Registry::UnLock(void)
{
  fIsReadOnly = false;
}
//____________________________________________________________________________
bool Registry::IsLocked(void) const
{
  return fIsReadOnly;
}
//____________________________________________________________________________
void Registry::InhibitItemLocks(void)
{
  fInhibitItemLocks = true;
}
//____________________________________________________________________________
void Registry::RestoreItemLocks(void)
{
  fInhibitItemLocks = false;
}
//____________________________________________________________________________
bool Registry::ItemLocksAreActive(void) const
{
  return !fInhibitItemLocks;
}
//____________________________________________________________________________
bool Registry::ItemIsLocked(RgKey key) const
{
  if( this->Exists(key) ) {
     RgIMapConstIter entry = fRegistry.find(key);
     bool is_locked = entry->second->IsLocked();
     return is_locked;
  }
  return false;
}
//____________________________________________________________________________
void Registry::LockItem(RgKey key)
{
  if( this->Exists(key) ) {
     RgIMapConstIter entry = fRegistry.find(key);
     entry->second->Lock();
  } else {
     LOG("Registry", pWARN)
           << "*** Can't lock non-existem item [" << key << "]";
  }
}
//____________________________________________________________________________
void Registry::UnLockItem(RgKey key)
{
  if( this->Exists(key) ) {
     RgIMapConstIter entry = fRegistry.find(key);
     entry->second->UnLock();
  } else {
    LOG("Registry", pWARN)
       << "*** Can't unlock non-existem item [" << key << "]";
  }
}
//____________________________________________________________________________
bool Registry::CanSetItem(RgKey key) const
{
  bool locked_item       = this->ItemIsLocked(key);
  bool active_item_locks = this->ItemLocksAreActive();
  bool locked_registry   = this->IsLocked();

  bool can_set = !locked_registry && ( !locked_item || !active_item_locks );

  return can_set;
}
//____________________________________________________________________________
void Registry::Set(RgIMapPair entry)
{
  RgKey key = entry.first;
  if( this->CanSetItem(key) ) {
    this->DeleteEntry(key);
    fRegistry.insert(entry);
  } else {
     LOG("Registry", pWARN)
             << "*** Registry item [" << key << "] can not be set";
  }
}
//____________________________________________________________________________
void Registry::Set(RgKey key, RgBool item)
{
  SetRegistryItem(this, key, item); // call templated set method
}
//____________________________________________________________________________
void Registry::Set(RgKey key, RgInt item)
{
  SetRegistryItem(this, key, item); // call templated set method
}
//____________________________________________________________________________
void Registry::Set(RgKey key, RgDbl item)
{
  SetRegistryItem(this, key, item); // call templated set method
}
//____________________________________________________________________________
void Registry::Set(RgKey key, RgCChAr item)
{
  RgStr item2 = RgStr(item); // "const char *" -> "string"
  this->Set(key, item2);
}
//____________________________________________________________________________
void Registry::Set(RgKey key, RgStr item)
{
  SetRegistryItem(this, key, item); // call templated set method
}
//____________________________________________________________________________
void Registry::Set(RgKey key, RgAlg item)
{
  SetRegistryItem(this, key, item); // call templated set method
}
//____________________________________________________________________________
void Registry::Set(RgKey key, RgH1F item)
{
  SetRegistryItem(this, key, item); // call templated set method
}
//____________________________________________________________________________
void Registry::Set(RgKey key, RgH2F item)
{
  SetRegistryItem(this, key, item); // call templated set method
}
//____________________________________________________________________________
void Registry::Set(RgKey key, RgTree item)
{
  SetRegistryItem(this, key, item); // call templated set method
}
//____________________________________________________________________________
void Registry::Get(RgKey key, const RegistryItemI * item) const
{
  RgIMapConstIter entry = fRegistry.find(key);
  item = entry->second;
}
//____________________________________________________________________________
void Registry::Get(RgKey key, RgBool & item) const
{
  LOG("Registry", pDEBUG) << "Get an RgBool item with key: " << key;

  RgIMapConstIter entry = fRegistry.find(key);
  RegistryItemI * rib = entry->second;
  RegistryItem<RgBool> * ri = dynamic_cast<RegistryItem<RgBool>*> (rib);

  LOG("Registry", pDEBUG) << "Item value = " << ri->Data();
  item = ri->Data();
}
//____________________________________________________________________________
void Registry::Get(RgKey key, RgInt & item) const
{
  LOG("Registry", pDEBUG) << "Getting an RgInt item with key: " << key;

  RgIMapConstIter entry = fRegistry.find(key);
  RegistryItemI * rib = entry->second;
  RegistryItem<RgInt> * ri = dynamic_cast< RegistryItem<RgInt> * > (rib);

  LOG("Registry", pDEBUG) << "Item value = " << ri->Data();
  item = ri->Data();
}
//____________________________________________________________________________
void Registry::Get(RgKey key, RgDbl & item) const
{
  LOG("Registry", pDEBUG) << "Getting an RgDbl item with key: " << key;

  RgIMapConstIter entry = fRegistry.find(key);
  RegistryItemI * rib = entry->second;
  RegistryItem<RgDbl> * ri = dynamic_cast<RegistryItem<RgDbl>*> (rib);

  LOG("Registry", pDEBUG) << "Item value = " << ri->Data();
  item = ri->Data();
}
//____________________________________________________________________________
void Registry::Get(RgKey key, RgStr & item) const
{
  LOG("Registry", pDEBUG) << "Getting an RgStr item with  key: " << key;

  RgIMapConstIter entry = fRegistry.find(key);
  RegistryItemI * rib = entry->second;
  RegistryItem<RgStr> * ri = dynamic_cast<RegistryItem<RgStr>*> (rib);

  LOG("Registry", pDEBUG) << "Item value = " << ri->Data();
  item = ri->Data();
}
//____________________________________________________________________________
void Registry::Get(RgKey key, RgAlg & item) const
{
  LOG("Registry", pDEBUG) << "Getting an RgAlg item with key: " << key;

  RgIMapConstIter entry = fRegistry.find(key);
  RegistryItemI * rib = entry->second;
  RegistryItem<RgAlg> * ri = dynamic_cast<RegistryItem<RgAlg>*> (rib);

  LOG("Registry", pDEBUG) << "Item value = " << ri->Data();
  item = ri->Data();
}
//____________________________________________________________________________
void Registry::Get(RgKey key, RgH1F & item) const
{
  LOG("Registry", pDEBUG) << "Getting an RgH1F item with key: " << key;

  RgIMapConstIter entry = fRegistry.find(key);
  RegistryItemI * rib = entry->second;
  RegistryItem<RgH1F> *ri = dynamic_cast<RegistryItem<RgH1F>*> (rib);
  item = ri->Data();

  if(!item) {
    LOG("Registry", pWARN) << "Returned NULL ptr for TH1F param = " << key;
  }
}
//____________________________________________________________________________
void Registry::Get(RgKey key, RgH2F & item) const
{
  LOG("Registry", pDEBUG) << "Getting an RgH2F item with key: " << key;

  RgIMapConstIter entry = fRegistry.find(key);
  RegistryItemI * rib = entry->second;
  RegistryItem<RgH2F> *ri = dynamic_cast<RegistryItem<RgH2F>*> (rib);
  item = ri->Data();

  if(!item) {
    LOG("Registry", pWARN) << "Returned NULL ptr for TH2F param = " << key;
  }
}
//____________________________________________________________________________
void Registry::Get(RgKey key, RgTree & item) const
{
  LOG("Registry", pDEBUG) << "Getting an RgTree item with key: " << key;

  RgIMapConstIter entry = fRegistry.find(key);
  RegistryItemI * rib = entry->second;
  RegistryItem<RgTree> *ri =  dynamic_cast<RegistryItem<RgTree>*> (rib);
  item = ri->Data();

  if(!item) {
    LOG("Registry", pWARN) << "Returned NULL ptr for TTree param = " << key;
  }
}
//____________________________________________________________________________
RgBool Registry::GetBool(RgKey key) const
{
  RgBool value;
  this->Get(key, value);
  return value;
}
//____________________________________________________________________________
RgInt Registry::GetInt(RgKey key) const
{
  RgInt value;
  this->Get(key, value);
  return value;
}
//____________________________________________________________________________
RgDbl Registry::GetDouble(RgKey key) const
{
  RgDbl value;
  this->Get(key, value);
  return value;
}
//____________________________________________________________________________
RgStr Registry::GetString(RgKey key) const
{
  RgStr value;
  this->Get(key, value);
  return value;
}
//____________________________________________________________________________
RgAlg Registry::GetAlg(RgKey key) const
{
  RgAlg value;
  this->Get(key, value);
  return value;
}
//____________________________________________________________________________
RgH1F Registry::GetH1F(RgKey key) const
{
  RgIMapConstIter entry = fRegistry.find(key);
  RegistryItemI * rib = entry->second;
  RegistryItem<RgH1F> *ri = dynamic_cast<RegistryItem<RgH1F>*> (rib);

  RgH1F item = ri->Data();
  return item;
}
//____________________________________________________________________________
RgH2F Registry::GetH2F(RgKey key) const
{
  RgIMapConstIter entry = fRegistry.find(key);
  RegistryItemI * rib = entry->second;
  RegistryItem<RgH2F> *ri = dynamic_cast<RegistryItem<RgH2F>*> (rib);

  RgH2F item = ri->Data();
  return item;
}
//____________________________________________________________________________
RgTree Registry::GetTree(RgKey key) const
{
  RgIMapConstIter entry = fRegistry.find(key);
  RegistryItemI * rib = entry->second;
  RegistryItem<RgTree> *ri = dynamic_cast<RegistryItem<RgTree>*> (rib);

  RgTree item = ri->Data();
  return item;
}
//____________________________________________________________________________
RgBool Registry::GetBoolDef(RgKey key, RgBool def_opt, bool set_def) 
{
  return GetValueOrUseDefault(this, key, def_opt, set_def);
}
//____________________________________________________________________________
int Registry::GetIntDef(RgKey key, int def_opt, bool set_def)
{
  return GetValueOrUseDefault(this, key, def_opt, set_def);
}
//____________________________________________________________________________
double Registry::GetDoubleDef(RgKey key, double def_opt, bool set_def) 
{
  return GetValueOrUseDefault(this, key, def_opt, set_def);
}
//____________________________________________________________________________
string Registry::GetStringDef(RgKey key, string def_opt, bool set_def) 
{
  return GetValueOrUseDefault(this, key, def_opt, set_def);
}
//____________________________________________________________________________
bool Registry::Exists(RgKey key) const
{
  if (fRegistry.count(key) == 1) return true;
  else                           return false;
}
//____________________________________________________________________________
bool Registry::DeleteEntry(RgKey key)
{
  if(!fIsReadOnly && Exists(key)) {
      RgIMapIter entry = fRegistry.find(key);
      RegistryItemI * item = entry->second;
      delete item;
      item = 0;
      fRegistry.erase(entry);
      return true;
  }
  return false;
}
//____________________________________________________________________________
int Registry::NEntries(void) const
{
  RgIMapSizeType reg_size = fRegistry.size();
  return (const int) reg_size;
}
//____________________________________________________________________________
void Registry::SetName(string name)
{
  if(! fIsReadOnly) fName = name;
  else {
     LOG("Registry", pWARN)
        << "*** Registry is locked - Can not change its name";
  }
}
//____________________________________________________________________________
string Registry::Name(void) const
{
  return fName;
}
//____________________________________________________________________________
void Registry::AssertExistence(RgKey key0) const
{
  if ( ! this->Exists(key0) ) {
     LOG("Registry", pERROR) << (*this);
     LOG("Registry", pFATAL)
           << "*** Key: " << key0
             << " does not exist in registry: " << this->Name();
     exit(1);    
  }
}
//____________________________________________________________________________
void Registry::AssertExistence(RgKey key0, RgKey key1) const
{
  this->AssertExistence(key0);
  this->AssertExistence(key1);
}
//____________________________________________________________________________
void Registry::AssertExistence(RgKey key0, RgKey key1, RgKey key2) const
{
  this->AssertExistence(key0);
  this->AssertExistence(key1);
  this->AssertExistence(key2);
}
//____________________________________________________________________________
void Registry::CopyToFolder(TFolder * folder) const
{
  LOG("Registry", pINFO) << "Converting Registry to TFolder";

  folder->SetOwner(true);

  RgIMapConstIter reg_iter;

  for(reg_iter = this->fRegistry.begin();
                          reg_iter != this->fRegistry.end(); reg_iter++) {

     ostringstream   entry;
     string          key   = reg_iter->first;
     RegistryItemI * ritem = reg_iter->second;
     string          type  = string(ritem->TypeInfo().name());

     entry << "key:" << key << ";type:" << type;

     if(type == "b") {
        entry << ";value: " << this->GetBool(key);
        LOG("Registry", pINFO) << "entry = " << entry.str();
        folder->Add(new TObjString(entry.str().c_str()));
     }
     else if (type == "d") {
        entry << ";value: " << this->GetDouble(key);
        LOG("Registry", pINFO) << "entry = " << entry.str();
        folder->Add(new TObjString(entry.str().c_str()));
     }
     else if (type == "i") {
        entry << ";value: " << this->GetInt(key);
        LOG("Registry", pINFO) << "entry = " << entry.str();
        folder->Add(new TObjString(entry.str().c_str()));
     }
     else if (type == "Ss") {
        entry << ";value: " << this->GetString(key);
        LOG("Registry", pINFO) << "entry = " << entry.str();
        folder->Add(new TObjString(entry.str().c_str()));
     }
     else if (type == "5RgAlg") {
//        entry << ";value: " << this->GetString(key);
//        LOG("Registry", pINFO) << "entry = " << entry.str();
//        folder->Add(new TObjString(entry.str().c_str()));
     }
     else if (type == "P4TH1F")
     {
     } else if (type == "P4TH2F") {
     } else if (type == "P4TTree") {
     } else {}
  }// registry iterator
}
//____________________________________________________________________________
void Registry::Print(ostream & stream) const
{
// Prints the registry to the specified stream
//
   stream << endl;
   stream << "[-] Registry name: [" << Name() << "]";

   stream << " - Write Status: ";
   if(fIsReadOnly)       { stream << "[locked]";   }
   else                  { stream << "[unlocked]"; }

   stream << " - Inhibited Item Locking: ";
   if(fInhibitItemLocks) { stream << "[on]";       }
   else                  { stream << "[off]";      }

   stream << " - # entries: " << setfill(' ') << setw(3) << fRegistry.size()
          << endl;

   RgIMapConstIter rcit = fRegistry.begin();
   for( ; rcit != fRegistry.end(); rcit++) {

     RgKey           key   = rcit->first;
     RegistryItemI * ritem = rcit->second;
     if(ritem) {
        string var_name = string("> ") + key;
        string var_type = string("[") + string(ritem->TypeInfo().name())
                        + string("] ");
        LOG("Registry", pDEBUG)
                    << "Printing " << var_type << " item named = " << key;
        stream << " |" << setfill('-') << setw(45) << var_name
               << setfill('.') << setw(12) << var_type;
        ritem->Print(stream);
        stream << endl;
     } else {
        LOG("Registry", pERROR) << "Null RegistryItemI with key = " << key;
     }
   }// registry iterator
}
//____________________________________________________________________________
void Registry::Copy(const Registry & registry)
{
// Copies the input registry
//

  if(this->IsLocked()) {
   LOG("Registry", pWARN) << "Registry is locked. Can't copy input entries!";
   return;
  }

  this->Init();
  this->Clear();
  this->Append(registry);

  fName             = registry.fName;
  fIsReadOnly       = registry.fIsReadOnly;
  fInhibitItemLocks = registry.fInhibitItemLocks;
}
//____________________________________________________________________________
void Registry::Append(const Registry & registry)
{
// Appends the input registry entries (& their locks)

  if(this->IsLocked()) {
   LOG("Registry", pWARN) << "Registry is locked. Can't copy input entries!";
   return;
  }

  this->InhibitItemLocks();

  RgIMapConstIter reg_iter;
  for(reg_iter = registry.fRegistry.begin();
                      reg_iter != registry.fRegistry.end(); reg_iter++) {

     RgKey          name = reg_iter->first;
     RegistryItemI * ri  = reg_iter->second;

     string var_type  = ri->TypeInfo().name();
     bool   item_lock = ri->IsLocked();

     LOG("Registry", pDEBUG)
                << "Copying [" << var_type << "] item named = " << name;

     RegistryItemI * cri = 0; // cloned registry item
     if (var_type == "b" )
           cri = new RegistryItem<RgBool>
                                (registry.GetBool(name),item_lock);
     else if (var_type == "d" )
           cri = new RegistryItem<RgDbl>
                                (registry.GetDouble(name),item_lock);
     else if (var_type == "i" )
           cri = new RegistryItem<RgInt>
                                (registry.GetInt(name),item_lock);
     else if (var_type == "Ss")
           cri = new RegistryItem<RgStr>
                                (registry.GetString(name),item_lock);
     else if (var_type == "5RgAlg")
           cri = new RegistryItem<RgAlg>
                                (registry.GetAlg(name),item_lock);
     else if (var_type == "P4TH1F") {
           RgH1F histo = registry.GetH1F(name);
           if(histo) {
               RgH1F chisto = new TH1F(*histo);
               LOG("Registry", pDEBUG) << chisto->GetName();
               cri = new RegistryItem<RgH1F>(chisto,item_lock);
           } else {
             LOG("Registry", pERROR)
               << "Null TH1F with key = " << name << " - not copied";
           }
     } else if (var_type == "P4TH2F") {
           RgH2F histo = registry.GetH2F(name);
           if(histo) {
               RgH2F chisto = new TH2F(*histo);
               LOG("Registry", pDEBUG) << chisto->GetName();
               cri = new RegistryItem<RgH2F>(chisto,item_lock);
           } else {
             LOG("Registry", pERROR)
               << "Null TH2F with key = " << name << " - not copied";
           }
     } else if (var_type == "P4TTree") {
           RgTree tree = registry.GetTree(name);
           if(tree) {
               //TTree * ctree = new TTree(*tree);
               TTree * ctree = tree->CopyTree("1");
               LOG("Registry", pDEBUG) << ctree->GetName();
               cri = new RegistryItem<RgTree>(ctree,item_lock);
           } else {
             LOG("Registry", pERROR)
               << "Null TTree with key = " << name << " - not copied";
           }
     } else {}

     RgIMapPair reg_entry(name, cri);
     fRegistry.insert(reg_entry);
   }
}
//____________________________________________________________________________
void Registry::Init(void)
{
// initialize registry properties

  fName              = "unnamed";
  fIsReadOnly        = false;
  fInhibitItemLocks  = false;
}
//____________________________________________________________________________
void Registry::Clear(bool force)
{
// clean all registry entries

  if(!force) {
   if(this->IsLocked()) {
      LOG("Registry", pWARN) << "Registry is locked. Can't clear its entries";
      return;
    }
  }
  RgIMapIter rit;
  for(rit = fRegistry.begin(); rit != fRegistry.end(); rit++) {
     RgKey           name = rit->first;
     RegistryItemI * item = rit->second;
     delete item;
     item = 0;
  }
  fRegistry.clear();
}
//____________________________________________________________________________

