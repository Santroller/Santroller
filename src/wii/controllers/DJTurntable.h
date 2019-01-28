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

#ifndef NXC_DJTurntable_h
#define NXC_DJTurntable_h

#include "../internal/ExtensionController.h"

#include "ClassicController.h"  // For joystick and +/- control maps

namespace NintendoExtensionCtrl {
	class DJTurntableController_Shared : public ExtensionController {
	public:
		struct Maps {
			constexpr static ByteMap JoyX = ClassicController_Shared::Maps::LeftJoyX;
			constexpr static ByteMap JoyY = ClassicController_Shared::Maps::LeftJoyY;

			constexpr static BitMap  ButtonPlus = ClassicController_Shared::Maps::ButtonPlus;
			constexpr static BitMap  ButtonMinus = ClassicController_Shared::Maps::ButtonMinus;

			constexpr static ByteMap Left_Turntable = ByteMap(3, 5, 0, 0);
			constexpr static ByteMap Left_TurntableSign = ByteMap(4, 1, 0, 0);
			constexpr static BitMap  Left_ButtonGreen = { 5, 3 };
			constexpr static BitMap  Left_ButtonRed = { 4, 5 };
			constexpr static BitMap  Left_ButtonBlue = { 5, 7 };

			constexpr static ByteMap Right_Turntable[3] = { ByteMap(0, 2, 6, 3), ByteMap(1, 2, 6, 5), ByteMap(2, 1, 7, 7) };
			constexpr static ByteMap Right_TurntableSign = ByteMap(2, 1, 0, 0);
			constexpr static BitMap  Right_ButtonGreen = { 5, 5 };
			constexpr static BitMap  Right_ButtonRed = { 4, 1 };
			constexpr static BitMap  Right_ButtonBlue = { 5, 2 };

			constexpr static ByteMap EffectDial[2] = { ByteMap(2, 2, 5, 2),  ByteMap(3, 3, 5, 5) };
			constexpr static ByteMap CrossfadeSlider = ByteMap(2, 4, 1, 1);

			constexpr static BitMap  ButtonEuphoria = { 5, 4 };
		};

		DJTurntableController_Shared(ExtensionData& dataRef) : 
			ExtensionController(dataRef, ExtensionType::DJTurntableController), left(*this), right(*this) {}

		DJTurntableController_Shared(ExtensionPort &port) :
			DJTurntableController_Shared(port.getExtensionData()) {}

		enum class TurntableConfig {
			BaseOnly,
			Left,
			Right,
			Both,
		};

		int8_t turntable() const;  // 6 bits, -30-29. Clockwise = positive, faster = larger.

		boolean buttonGreen() const;
		boolean buttonRed() const;
		boolean buttonBlue() const;

		uint8_t effectDial() const;  // 5 bits, 0-31. One rotation per rollover.
		int8_t crossfadeSlider() const;  // 4 bits, -8-7. Negative to the left.

		boolean buttonEuphoria() const;

		uint8_t joyX() const;  // 6 bits, 0-63
		uint8_t joyY() const;

		boolean buttonPlus() const;
		boolean buttonMinus() const;

		void printDebug();

		TurntableConfig getTurntableConfig();
		uint8_t getNumTurntables();

		class TurntableExpansion {
		public:
			TurntableExpansion(TurntableConfig conf, DJTurntableController_Shared &baseObj)
				: side(conf), base(baseObj) {}
			boolean connected() const;

			virtual int8_t turntable() const = 0;

			virtual boolean buttonGreen() const = 0;
			virtual boolean buttonRed() const = 0;
			virtual boolean buttonBlue() const = 0;

			const TurntableConfig side = TurntableConfig::BaseOnly;
		protected:
			int8_t getTurntableSpeed(uint8_t turnData, boolean turnSign) const {
				if (turnSign) {  // If sign bit is 1...
					turnData |= 0xE0;  // Flip all sign-related bits to '1's
				}
				return (int8_t) turnData;
			}

			const DJTurntableController_Shared & base;
		};

		class TurntableLeft : public TurntableExpansion {
		public:
			TurntableLeft(DJTurntableController_Shared &baseObj)
				: TurntableExpansion(TurntableConfig::Left, baseObj) {}
			int8_t turntable() const;

			boolean buttonGreen() const;
			boolean buttonRed() const;
			boolean buttonBlue()  const;
		} left;

		class TurntableRight : public TurntableExpansion {
		public:
			TurntableRight(DJTurntableController_Shared &baseObj)
				: TurntableExpansion(TurntableConfig::Right, baseObj) {}
			int8_t turntable() const;

			boolean buttonGreen() const;
			boolean buttonRed() const;
			boolean buttonBlue() const;
		} right;

		class EffectRollover : private NintendoExtensionCtrl::RolloverChange {
		public:
			EffectRollover(DJTurntableController_Shared & controller) : RolloverChange(0, 31), dj(controller) {}
			int8_t getChange();
		private:
			const DJTurntableController_Shared & dj;
		};

	private:
		void printTurntable(TurntableExpansion &table) const;

		TurntableConfig tableConfig = TurntableConfig::BaseOnly;
	};
}

using DJTurntableController = NintendoExtensionCtrl::BuildControllerClass
	<NintendoExtensionCtrl::DJTurntableController_Shared>;

#endif
