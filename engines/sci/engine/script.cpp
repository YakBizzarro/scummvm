/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#include "sci/sci.h"
#include "sci/resource.h"
#include "sci/util.h"
#include "sci/engine/features.h"
#include "sci/engine/state.h"
#include "sci/engine/kernel.h"
#include "sci/engine/script.h"

#include "common/util.h"

namespace Sci {

#define END Script_None

opcode_format g_opcode_formats[128][4] = {
	/*00*/
	{Script_None}, {Script_None}, {Script_None}, {Script_None},
	/*04*/
	{Script_None}, {Script_None}, {Script_None}, {Script_None},
	/*08*/
	{Script_None}, {Script_None}, {Script_None}, {Script_None},
	/*0C*/
	{Script_None}, {Script_None}, {Script_None}, {Script_None},
	/*10*/
	{Script_None}, {Script_None}, {Script_None}, {Script_None},
	/*14*/
	{Script_None}, {Script_None}, {Script_None}, {Script_SRelative, END},
	/*18*/
	{Script_SRelative, END}, {Script_SRelative, END}, {Script_SVariable, END}, {Script_None},
	/*1C*/
	{Script_SVariable, END}, {Script_None}, {Script_None}, {Script_Variable, END},
	/*20*/
	{Script_SRelative, Script_Byte, END}, {Script_Variable, Script_Byte, END}, {Script_Variable, Script_Byte, END}, {Script_Variable, Script_SVariable, Script_Byte, END},
	/*24 (24=ret)*/
	{Script_End}, {Script_Byte, END}, {Script_Invalid}, {Script_Invalid},
	/*28*/
	{Script_Variable, END}, {Script_Invalid}, {Script_Byte, END}, {Script_Variable, Script_Byte, END},
	/*2C*/
	{Script_SVariable, END}, {Script_SVariable, Script_Variable, END}, {Script_None}, {Script_Invalid},
	/*30*/
	{Script_None}, {Script_Property, END}, {Script_Property, END}, {Script_Property, END},
	/*34*/
	{Script_Property, END}, {Script_Property, END}, {Script_Property, END}, {Script_Property, END},
	/*38*/
	{Script_Property, END}, {Script_SRelative, END}, {Script_SRelative, END}, {Script_None},
	/*3C*/
	{Script_None}, {Script_None}, {Script_None}, {Script_Word},
	/*40-4F*/
	{Script_Global, END}, {Script_Local, END}, {Script_Temp, END}, {Script_Param, END},
	{Script_Global, END}, {Script_Local, END}, {Script_Temp, END}, {Script_Param, END},
	{Script_Global, END}, {Script_Local, END}, {Script_Temp, END}, {Script_Param, END},
	{Script_Global, END}, {Script_Local, END}, {Script_Temp, END}, {Script_Param, END},
	/*50-5F*/
	{Script_Global, END}, {Script_Local, END}, {Script_Temp, END}, {Script_Param, END},
	{Script_Global, END}, {Script_Local, END}, {Script_Temp, END}, {Script_Param, END},
	{Script_Global, END}, {Script_Local, END}, {Script_Temp, END}, {Script_Param, END},
	{Script_Global, END}, {Script_Local, END}, {Script_Temp, END}, {Script_Param, END},
	/*60-6F*/
	{Script_Global, END}, {Script_Local, END}, {Script_Temp, END}, {Script_Param, END},
	{Script_Global, END}, {Script_Local, END}, {Script_Temp, END}, {Script_Param, END},
	{Script_Global, END}, {Script_Local, END}, {Script_Temp, END}, {Script_Param, END},
	{Script_Global, END}, {Script_Local, END}, {Script_Temp, END}, {Script_Param, END},
	/*70-7F*/
	{Script_Global, END}, {Script_Local, END}, {Script_Temp, END}, {Script_Param, END},
	{Script_Global, END}, {Script_Local, END}, {Script_Temp, END}, {Script_Param, END},
	{Script_Global, END}, {Script_Local, END}, {Script_Temp, END}, {Script_Param, END},
	{Script_Global, END}, {Script_Local, END}, {Script_Temp, END}, {Script_Param, END}
};
#undef END

// TODO: script_adjust_opcode_formats should probably be part of the
// constructor (?) of a VirtualMachine or a ScriptManager class.
void script_adjust_opcode_formats(EngineState *s) {
	// TODO: Check that this is correct
	if (g_sci->_features->detectLofsType() != SCI_VERSION_0_EARLY) {
		g_opcode_formats[op_lofsa][0] = Script_Offset;
		g_opcode_formats[op_lofss][0] = Script_Offset;
	}

#ifdef ENABLE_SCI32
	// In SCI32, some arguments are now words instead of bytes
	if (getSciVersion() >= SCI_VERSION_2) {
		g_opcode_formats[op_calle][2] = Script_Word;
		g_opcode_formats[op_callk][1] = Script_Word;
		g_opcode_formats[op_super][1] = Script_Word;
		g_opcode_formats[op_send][0] = Script_Word;
		g_opcode_formats[op_self][0] = Script_Word;
		g_opcode_formats[op_call][1] = Script_Word;
		g_opcode_formats[op_callb][1] = Script_Word;
	}
#endif
}

void SegManager::createClassTable() {
	Resource *vocab996 = _resMan->findResource(ResourceId(kResourceTypeVocab, 996), 1);

	if (!vocab996)
		error("SegManager: failed to open vocab 996");

	int totalClasses = vocab996->size >> 2;
	_classTable.resize(totalClasses);

	for (uint16 classNr = 0; classNr < totalClasses; classNr++) {
		uint16 scriptNr = READ_SCI11ENDIAN_UINT16(vocab996->data + classNr * 4 + 2);

		_classTable[classNr].reg = NULL_REG;
		_classTable[classNr].script = scriptNr;
	}

	_resMan->unlockResource(vocab996);
}

reg_t SegManager::getClassAddress(int classnr, ScriptLoadType lock, reg_t caller) {
	if (classnr == 0xffff)
		return NULL_REG;

	if (classnr < 0 || (int)_classTable.size() <= classnr || _classTable[classnr].script < 0) {
		error("[VM] Attempt to dereference class %x, which doesn't exist (max %x)", classnr, _classTable.size());
		return NULL_REG;
	} else {
		Class *the_class = &_classTable[classnr];
		if (!the_class->reg.segment) {
			getScriptSegment(the_class->script, lock);

			if (!the_class->reg.segment) {
				error("[VM] Trying to instantiate class %x by instantiating script 0x%x (%03d) failed;", classnr, the_class->script, the_class->script);
				return NULL_REG;
			}
		} else
			if (caller.segment != the_class->reg.segment)
				getScript(the_class->reg.segment)->incrementLockers();

		return the_class->reg;
	}
}

void SegManager::scriptInitialiseLocalsZero(SegmentId seg, int count) {
	Script *scr = getScript(seg);

	scr->_localsOffset = -count * 2; // Make sure it's invalid

	LocalVariables *locals = allocLocalsSegment(scr, count);
	if (locals) {
		for (int i = 0; i < count; i++)
			locals->_locals[i] = NULL_REG;
	}
}

void SegManager::scriptInitialiseLocals(reg_t location) {
	Script *scr = getScript(location.segment);
	unsigned int count;

	VERIFY(location.offset + 1 < (uint16)scr->getBufSize(), "Locals beyond end of script\n");

	if (getSciVersion() >= SCI_VERSION_1_1)
		count = READ_SCI11ENDIAN_UINT16(scr->_buf + location.offset - 2);
	else
		count = (READ_LE_UINT16(scr->_buf + location.offset - 2) - 4) >> 1;
	// half block size

	scr->_localsOffset = location.offset;

	if (!(location.offset + count * 2 + 1 < scr->getBufSize())) {
		warning("Locals extend beyond end of script: offset %04x, count %x vs size %x", location.offset, count, (uint)scr->getBufSize());
		count = (scr->getBufSize() - location.offset) >> 1;
	}

	LocalVariables *locals = allocLocalsSegment(scr, count);
	if (locals) {
		uint i;
		const byte *base = (const byte *)(scr->_buf + location.offset);

		for (i = 0; i < count; i++)
			locals->_locals[i] = make_reg(0, READ_SCI11ENDIAN_UINT16(base + i * 2));
	}
}

void SegManager::scriptInitialiseObjectsSci11(SegmentId seg) {
	Script *scr = getScript(seg);
	const byte *seeker = scr->_heapStart + 4 + READ_SCI11ENDIAN_UINT16(scr->_heapStart + 2) * 2;

	while (READ_SCI11ENDIAN_UINT16(seeker) == SCRIPT_OBJECT_MAGIC_NUMBER) {
		if (READ_SCI11ENDIAN_UINT16(seeker + 14) & SCRIPT_INFO_CLASS) {	// -info- selector
			int classpos = seeker - scr->_buf;
			int species = READ_SCI11ENDIAN_UINT16(seeker + 10);

			if (species < 0 || species >= (int)_classTable.size()) {
				error("Invalid species %d(0x%x) not in interval [0,%d) while instantiating script %d",
				          species, species, _classTable.size(), scr->_nr);
				return;
			}

			_classTable[species].reg.segment = seg;
			_classTable[species].reg.offset = classpos;
		}
		seeker += READ_SCI11ENDIAN_UINT16(seeker + 2) * 2;
	}

	seeker = scr->_heapStart + 4 + READ_SCI11ENDIAN_UINT16(scr->_heapStart + 2) * 2;
	while (READ_SCI11ENDIAN_UINT16(seeker) == SCRIPT_OBJECT_MAGIC_NUMBER) {
		reg_t reg = make_reg(seg, seeker - scr->_buf);
		Object *obj = scr->scriptObjInit(reg);

		// Copy base from species class, as we need its selector IDs
		obj->setSuperClassSelector(
			getClassAddress(obj->getSuperClassSelector().offset, SCRIPT_GET_LOCK, NULL_REG));

		// If object is instance, get -propDict- from class and set it for this object
		//  This is needed for ::isMemberOf() to work.
		// Example testcase - room 381 of sq4cd - if isMemberOf() doesn't work, talk-clicks on the robot will act like
		//                     clicking on ego
		if (!obj->isClass()) {
			reg_t classObject = obj->getSuperClassSelector();
			Object *classObj = getObject(classObject);
			obj->setPropDictSelector(classObj->getPropDictSelector());
		}

		// Set the -classScript- selector to the script number.
		// FIXME: As this selector is filled in at run-time, it is likely
		// that it is supposed to hold a pointer. The Obj::isKindOf method
		// uses this selector together with -propDict- to compare classes.
		// For the purpose of Obj::isKindOf, using the script number appears
		// to be sufficient.
		obj->setClassScriptSelector(make_reg(0, scr->_nr));

		seeker += READ_SCI11ENDIAN_UINT16(seeker + 2) * 2;
	}
}



int script_instantiate_common(ResourceManager *resMan, SegManager *segMan, int script_nr, int *was_new) {
	*was_new = 1;

	SegmentId seg_id = segMan->getScriptSegment(script_nr);
	Script *scr = segMan->getScriptIfLoaded(seg_id);
	if (scr) {
		if (!scr->isMarkedAsDeleted()) {
			scr->incrementLockers();
			return seg_id;
		} else {
			scr->freeScript();
		}
	} else {
		scr = segMan->allocateScript(script_nr, &seg_id);
	}

	scr->init(script_nr, resMan);
	scr->load(resMan);

	*was_new = 0;

	return seg_id;
}

int script_instantiate_sci0(ResourceManager *resMan, SegManager *segMan, int script_nr) {
	int objType;
	uint32 objLength = 0;
	int relocation = -1;
	int was_new;
	bool oldScriptHeader = (getSciVersion() == SCI_VERSION_0_EARLY);
	const int seg_id = script_instantiate_common(resMan, segMan, script_nr, &was_new);
	uint16 curOffset = oldScriptHeader ? 2 : 0;

	if (was_new)
		return seg_id;

	Script *scr = segMan->getScript(seg_id);

	if (oldScriptHeader) {
		// Old script block
		// There won't be a localvar block in this case
		// Instead, the script starts with a 16 bit int specifying the
		// number of locals we need; these are then allocated and zeroed.
		int localsCount = READ_LE_UINT16(scr->_buf);
		if (localsCount)
			segMan->scriptInitialiseLocalsZero(seg_id, localsCount);
	}

	// Now do a first pass through the script objects to find the
	// local variable blocks

	do {
		objType = scr->getHeap(curOffset);
		if (!objType)
			break;

		objLength = scr->getHeap(curOffset + 2);
		curOffset += 4;		// skip header

		switch (objType) {
		case SCI_OBJ_LOCALVARS:
			segMan->scriptInitialiseLocals(make_reg(seg_id, curOffset));
			break;
		case SCI_OBJ_CLASS: {
			int classpos = curOffset - SCRIPT_OBJECT_MAGIC_OFFSET;
			int species = scr->getHeap(curOffset - SCRIPT_OBJECT_MAGIC_OFFSET + SCRIPT_SPECIES_OFFSET);
			if (species < 0 || species >= (int)segMan->classTableSize()) {
				if (species == (int)segMan->classTableSize()) {
					// Happens in the LSL2 demo
					warning("Applying workaround for an off-by-one invalid species access");
					segMan->resizeClassTable(segMan->classTableSize() + 1);
				} else {
					warning("Invalid species %d(0x%x) not in interval "
							  "[0,%d) while instantiating script %d\n",
							  species, species, segMan->classTableSize(),
							  script_nr);
					return 0;
				}
			}

			segMan->setClassOffset(species, make_reg(seg_id, classpos));
			// Set technical class position-- into the block allocated for it
		}
		break;

		default:
			break;
		}

		curOffset += objLength - 4;
	} while (objType != 0 && curOffset < scr->getScriptSize() - 2);

	// And now a second pass to adjust objects and class pointers, and the general pointers
	objLength = 0;
	curOffset = oldScriptHeader ? 2 : 0;

	do {
		objType = scr->getHeap(curOffset);
		if (!objType)
			break;

		objLength = scr->getHeap(curOffset + 2);
		curOffset += 4;		// skip header

		reg_t addr = make_reg(seg_id, curOffset);

		switch (objType) {
		case SCI_OBJ_CODE:
			scr->scriptAddCodeBlock(addr);
			break;
		case SCI_OBJ_OBJECT:
		case SCI_OBJ_CLASS: { // object or class?
			Object *obj = scr->scriptObjInit(addr);
			obj->initSpecies(segMan, addr);

			if (!obj->initBaseObject(segMan, addr)) {
				warning("Failed to locate base object for object at %04X:%04X; skipping", PRINT_REG(addr));
				scr->scriptObjRemove(addr);
			}
		} // if object or class
		break;
		case SCI_OBJ_POINTERS: // A relocation table
			relocation = addr.offset;
			break;

		default:
			break;
		}

		curOffset += objLength - 4;
	} while (objType != 0 && curOffset < scr->getScriptSize() - 2);

	if (relocation >= 0)
		scr->relocate(make_reg(seg_id, relocation));

	return seg_id;		// instantiation successful
}

int script_instantiate_sci11(ResourceManager *resMan, SegManager *segMan, int script_nr) {
	int was_new;
	const int seg_id = script_instantiate_common(resMan, segMan, script_nr, &was_new);

	if (was_new)
		return seg_id;

	Script *scr = segMan->getScript(seg_id);

	int heapStart = scr->getScriptSize();
	segMan->scriptInitialiseLocals(make_reg(seg_id, heapStart + 4));
	segMan->scriptInitialiseObjectsSci11(seg_id);
	scr->relocate(make_reg(seg_id, READ_SCI11ENDIAN_UINT16(scr->_heapStart)));

	return seg_id;
}

int script_instantiate(ResourceManager *resMan, SegManager *segMan, int script_nr) {
	if (getSciVersion() >= SCI_VERSION_1_1)
		return script_instantiate_sci11(resMan, segMan, script_nr);
	else
		return script_instantiate_sci0(resMan, segMan, script_nr);
}

void script_uninstantiate_sci0(SegManager *segMan, int script_nr, SegmentId seg) {
	bool oldScriptHeader = (getSciVersion() == SCI_VERSION_0_EARLY);
	reg_t reg = make_reg(seg, oldScriptHeader ? 2 : 0);
	int objType, objLength = 0;
	Script *scr = segMan->getScript(seg);

	// Make a pass over the object in order uninstantiate all superclasses

	do {
		reg.offset += objLength; // Step over the last checked object

		objType = scr->getHeap(reg.offset);
		if (!objType)
			break;
		objLength = scr->getHeap(reg.offset + 2);  // use SEG_UGET_HEAP ??

		reg.offset += 4; // Step over header

		if ((objType == SCI_OBJ_OBJECT) || (objType == SCI_OBJ_CLASS)) { // object or class?
			int superclass;

			reg.offset -= SCRIPT_OBJECT_MAGIC_OFFSET;

			superclass = scr->getHeap(reg.offset + SCRIPT_SUPERCLASS_OFFSET); // Get superclass...

			if (superclass >= 0) {
				int superclass_script = segMan->getClass(superclass).script;

				if (superclass_script == script_nr) {
					if (scr->getLockers())
						scr->decrementLockers();  // Decrease lockers if this is us ourselves
				} else
					script_uninstantiate(segMan, superclass_script);
				// Recurse to assure that the superclass lockers number gets decreased
			}

			reg.offset += SCRIPT_OBJECT_MAGIC_OFFSET;
		} // if object or class

		reg.offset -= 4; // Step back on header

	} while (objType != 0);
}

void script_uninstantiate(SegManager *segMan, int script_nr) {
	SegmentId segment = segMan->getScriptSegment(script_nr);
	Script *scr = segMan->getScriptIfLoaded(segment);

	if (!scr) {   // Is it already loaded?
		//warning("unloading script 0x%x requested although not loaded", script_nr);
		// This is perfectly valid SCI behaviour
		return;
	}

	scr->decrementLockers();   // One less locker

	if (scr->getLockers() > 0)
		return;

	// Free all classtable references to this script
	for (uint i = 0; i < segMan->classTableSize(); i++)
		if (segMan->getClass(i).reg.segment == segment)
			segMan->setClassOffset(i, NULL_REG);

	if (getSciVersion() < SCI_VERSION_1_1)
		script_uninstantiate_sci0(segMan, script_nr, segment);
	// FIXME: Add proper script uninstantiation for SCI 1.1

	if (scr->getLockers())
		return; // if xxx.lockers > 0

	// Otherwise unload it completely
	// Explanation: I'm starting to believe that this work is done by SCI itself.
	scr->markDeleted();

	debugC(kDebugLevelScripts, "Unloaded script 0x%x.", script_nr);

	return;
}


} // End of namespace Sci
