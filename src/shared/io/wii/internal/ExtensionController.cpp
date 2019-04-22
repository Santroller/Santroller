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

#include "ExtensionController.h"
#include <string.h>
using namespace NintendoExtensionCtrl;

ExtensionController::ExtensionController(ExtensionData& dataRef)
	: ExtensionController(dataRef, ExtensionType::AnyController) {}

ExtensionController::ExtensionController(ExtensionData& dataRef, ExtensionType conID)
	: id(conID), data(dataRef)  {}

void ExtensionController::begin() {
	 // Initialize the bus
}

boolean ExtensionController::connect() {
	disconnect();  // Clear current data
	return reconnect();
}

boolean ExtensionController::reconnect() {
	boolean success = false;

	if (initialize()) {
		identifyController();
		success = update();  // Seed with initial values
	}
	else {
		data.connectedType = ExtensionType::NoController;  // Bad init, nothing connected
	}

	return success;
}

void ExtensionController::disconnect() {
	data.connectedType = ExtensionType::NoController;  // Nothing connected
	memset(&data.controlData, 0x00, ExtensionData::ControlDataSize);  // Clear control data
}

void ExtensionController::reset() {
	disconnect();
	requestSize = MinRequestSize;  // Request size back to minimum
}

void ExtensionController::identifyController() {
	data.connectedType = NintendoExtensionCtrl::identifyController();  // Polls the controller for its identity
}

boolean ExtensionController::controllerIDMatches() const {
	if (data.connectedType == id) {
		return true;  // Match!
	}
	else if (id == ExtensionType::AnyController && data.connectedType != ExtensionType::NoController) {
		return true;  // No enforcing and some sort of controller connected
	}

	return false;  // Enforced types or no controller connected
}

ExtensionType ExtensionController::getControllerType() const {
	return data.connectedType;
}

boolean ExtensionController::update() {
	if (controllerIDMatches() && requestControlData(requestSize, data.controlData)) {
		return verifyData(data.controlData, requestSize);
	}
	
	return false;  // Something went wrong :(
}

uint8_t ExtensionController::getControlData(uint8_t controlIndex) const {
	return data.controlData[controlIndex];
}

void ExtensionController::setControlData(uint8_t index, uint8_t val) {
	data.controlData[index] = val;
}

ExtensionController::ExtensionData & ExtensionController::getExtensionData() const {
	return data;
}

void ExtensionController::setRequestSize(size_t r) {
	if (r >= MinRequestSize && r <= MaxRequestSize) {
		requestSize = (uint8_t) r;
	}
}

