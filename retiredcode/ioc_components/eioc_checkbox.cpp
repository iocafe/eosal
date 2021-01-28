/**

  @file    ecomponent.cpp
  @brief   Abstract GUI component.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.9.2020

  Base call for all GUI components (widgets)...

  Copyright 2020 Pekka Lehtikoski. This file is part of the eobjects project and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "egui.h"


/**
****************************************************************************************************

  @brief Constructor.

  X...

  @return  None.

****************************************************************************************************
*/
eiocCheckbox::eiocCheckbox(
    eObject *parent,
    e_oid id,
    os_int flags)
    : eComponent(parent, id, flags)
{
}


/**
****************************************************************************************************

  @brief Virtual destructor.

  X...

  @return  None.

****************************************************************************************************
*/
eiocCheckbox::~eiocCheckbox()
{
}


/**
****************************************************************************************************

  @brief Clone object

  The clone function clones the variable and clonable attachments. Names will be left detached
  in clone if EOBJ_NO_MAP flag is given.

  @param  parent Parent for the clone.
  @param  id Object identifier for the clone.
  @param  aflags 0 for default operation. EOBJ_NO_MAP not to map names.
  @return Pointer to the clone.

****************************************************************************************************
*/
eObject *eiocCheckbox::clone(
    eObject *parent,
    e_oid id,
    os_int aflags)
{
    eiocCheckbox *clonedobj;
    clonedobj = new eiocCheckbox(parent, id == EOID_CHILD ? oid() : id, flags());
    clonegeneric(clonedobj, aflags);
    return clonedobj;
}


/**
****************************************************************************************************

  @brief Add eiocCheckbox to class list and class'es properties to it's property set.

  The eiocCheckbox::setupclass function adds eiocCheckbox to class list and class'es properties to
  it's property set. The class list enables creating new objects dynamically by class identifier,
  which is used for serialization reader functions. The property set stores static list of
  class'es properties and metadata for those.

****************************************************************************************************
*/
void eiocCheckbox::setupclass()
{
    const os_int cls = EGUICLASSID_IOC_CHECKBOX;

    os_lock();
    eclasslist_add(cls, (eNewObjFunc)newobj, "eiocCheckbox");
    setupproperties(cls, ECOMP_NO_OPTIONAL_PROPERITES);
    propertysetdone(cls);
    os_unlock();
}


/**
****************************************************************************************************

  @brief Draw the component.

  The eiocCheckbox::draw() function calls ImGui API to render the component.

  @param   Prm Drawing parameters.
  @return  The function return ESTATUS_SUCCESS if all is fine. Other values indicate that the
           component is no longer drawable or useful. This could be for example a pop up menu
           closed implicitely by clicking elsewhere.

****************************************************************************************************
*/
eStatus eiocCheckbox::draw(
    eDrawParams& prm)
{
    add_to_zorder(prm.window, prm.layer);

    ImGui::Text("HERREE.");

    return ESTATUS_SUCCESS;
}

