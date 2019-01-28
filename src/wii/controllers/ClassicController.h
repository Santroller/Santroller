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

#ifndef NXC_ClassicController_h
#define NXC_ClassicController_h

#include "../internal/ExtensionController.h"

namespace NintendoExtensionCtrl {
	class ClassicController_Shared : public ExtensionController {
	public:
		struct Maps {
			constexpr static ByteMap LeftJoyX = ByteMap(0, 6, 0, 0);
			constexpr static ByteMap LeftJoyY = ByteMap(1, 6, 0, 0);

			constexpr static ByteMap RightJoyX[3] = { ByteMap(0, 2, 6, 3), ByteMap(1, 2, 6, 5), ByteMap(2, 1, 7, 7) };
			constexpr static ByteMap RightJoyY = ByteMap(2, 5, 0, 0);

			constexpr static BitMap  DpadUp = { 5, 0 };
			constexpr static BitMap  DpadDown = { 4, 6 };
			constexpr static BitMap  DpadLeft = { 5, 1 };
			constexpr static BitMap  DpadRight = { 4, 7 };

			constexpr static BitMap  ButtonA = { 5, 4 };
			constexpr static BitMap  ButtonB = { 5, 6 };
			constexpr static BitMap  ButtonX = { 5, 3 };
			constexpr static BitMap  ButtonY = { 5, 5 };

			constexpr static ByteMap TriggerL[2] = { ByteMap(2, 2, 5, 2), ByteMap(3, 3, 5, 5) };
			constexpr static ByteMap TriggerR = ByteMap(3, 5, 0, 0);

			constexpr static BitMap  ButtonL = { 4, 5 };
			constexpr static BitMap  ButtonR = { 4, 1 };
			constexpr static BitMap  ButtonZL = { 5, 7 };
			constexpr static BitMap  ButtonZR = { 5, 2 };

			constexpr static BitMap  ButtonPlus = { 4, 2 };
			constexpr static BitMap  ButtonMinus = { 4, 4 };
			constexpr static BitMap  ButtonHome = { 4, 3 };
		};

		ClassicController_Shared(ExtensionData &dataRef) :
			ExtensionController(dataRef, ExtensionType::ClassicController) {}

		ClassicController_Shared(ExtensionPort &port) :
			ClassicController_Shared(port.getExtensionData()) {}

		uint8_t leftJoyX() const;  // 6 bits, 0-63
		uint8_t leftJoyY() const;

		uint8_t rightJoyX() const;  // 5 bits, 0-31
		uint8_t rightJoyY() const;

		boolean dpadUp() const;
		boolean dpadDown() const;
		boolean dpadLeft() const;
		boolean dpadRight() const;

		boolean buttonA() const;
		boolean buttonB() const;
		boolean buttonX() const;
		boolean buttonY() const;

		uint8_t triggerL() const;  // 5 bits, 0-31
		uint8_t triggerR() const;

		boolean buttonL() const;
		boolean buttonR() const;

		boolean buttonZL() const;
		boolean buttonZR() const;

		boolean buttonStart() const;
		boolean buttonSelect() const;

		boolean buttonPlus() const;
		boolean buttonMinus() const;

		boolean buttonHome() const;

	// NES Knockoff Support
	public:
		boolean isNESKnockoff() const;
		boolean fixNESKnockoffData();

	protected:
		void manipulateKnockoffData();
	};

	class NESMiniController_Shared : public ClassicController_Shared {
	public:
		using ClassicController_Shared::ClassicController_Shared;

		boolean isKnockoff() const { return isNESKnockoff(); }
		boolean fixKnockoffData() { return fixNESKnockoffData(); }
	};

	class SNESMiniController_Shared : public ClassicController_Shared {
	public:
		using ClassicController_Shared::ClassicController_Shared;
	};
}

using ClassicController = NintendoExtensionCtrl::BuildControllerClass
	<NintendoExtensionCtrl::ClassicController_Shared>;

using NESMiniController = NintendoExtensionCtrl::BuildControllerClass
	<NintendoExtensionCtrl::NESMiniController_Shared>;

using SNESMiniController = NintendoExtensionCtrl::BuildControllerClass
	<NintendoExtensionCtrl::SNESMiniController_Shared>;

#endif
