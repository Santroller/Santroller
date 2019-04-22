/*
*  Project     Nintendo Extension Controller Library
*  @author     David Madison
*  @link       github.com/dmadison/NintendoExtensionCtrl
*  @license    LGPLv3 - Copyright (c) 2018 David Madison
*
*  This file is part of the Nintendo Extension Controller Library.
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU Lesser General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU Lesser General Public License for more details.
*
*  You should have received a copy of the GNU Lesser General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef NXC_ExtensionController_h
#define NXC_ExtensionController_h

#include "NXC_Identity.h"
#include "NXC_Comms.h"
#include "NXC_Utils.h"
#include "NXC_DataMaps.h"
#include "stdlib.h"

class ExtensionController {
public:
	struct ExtensionData {
		friend class ExtensionController;

		static const uint8_t ControlDataSize = 21;  // Largest reporting mode (0x3d)

	protected:
		ExtensionType connectedType = ExtensionType::NoController;
		uint8_t controlData[ControlDataSize];
	};

	ExtensionController(ExtensionData& dataRef);

	void begin();

	boolean connect();
	boolean reconnect();

	boolean update();

	void reset();

	ExtensionType getControllerType() const;
	uint8_t getControlData(uint8_t controlIndex) const;
	ExtensionData & getExtensionData() const;

	void setRequestSize(size_t size = MinRequestSize);

	void printDebug() const;
	void printDebugID() const;
	void printDebugRaw() const;
	void printDebugRaw(uint8_t baseFormat) const;

	static const uint8_t MinRequestSize = 6;   // Smallest reporting mode (0x37)
	static const uint8_t MaxRequestSize = ExtensionData::ControlDataSize;

	const ExtensionType id = ExtensionType::AnyController;

protected:
	ExtensionController(ExtensionData& dataRef, ExtensionType conID);

	typedef NintendoExtensionCtrl::CtrlIndex CtrlIndex;
	typedef NintendoExtensionCtrl::ByteMap   ByteMap;
	typedef NintendoExtensionCtrl::BitMap    BitMap;

	uint8_t getControlData(const ByteMap map) const {
		return (data.controlData[map.index] & map.mask) >> map.offset;
	}

	template<size_t size>
	uint8_t getControlData(const ByteMap(&map)[size]) const {
		uint8_t dataOut = 0x00;
		for (size_t i = 0; i < size; i++) {
			/* Repeated line from the single-ByteMap function above. Apparently the
				constexpr stuff doesn't like being passed through nested functions. */
			dataOut |= (data.controlData[map[i].index] & map[i].mask) >> map[i].offset;
			//dataOut |= getControlData(map[i]);
		}
		return dataOut;
	}

	boolean getControlBit(const BitMap map) const {
		return !(data.controlData[map.index] & (1 << map.position));  // Inverted logic, '0' is pressed
	}

	void setControlData(uint8_t index, uint8_t val);

private:
	ExtensionData &data;  // I2C and control data storage

	void disconnect();
	void identifyController();
	boolean controllerIDMatches() const;

	uint8_t requestSize = MinRequestSize;
};

namespace NintendoExtensionCtrl {
	template <class ControllerMap>
	class BuildControllerClass : public ControllerMap {
	public:
		BuildControllerClass() :
			ControllerMap(portData),
			portData() {}

		using Shared = ControllerMap;  // Make controller class easily accessible

	protected:
		// Included data instance. Contains:
		//    * I2C library object reference
		//    * Connected controller identity (type)
		//    * Control data array
		// This data can be shared between controller instances using a single
		// logical endpoint to keep memory down.
		ExtensionController::ExtensionData portData;
	};
}

// Public-facing version of the extension 'port' class that combines the 
// communication (ExtensionController) with a data instance (ExtensionData), but omits
// any controller-specific data maps.
using ExtensionPort = NintendoExtensionCtrl::BuildControllerClass<ExtensionController>;

#endif
