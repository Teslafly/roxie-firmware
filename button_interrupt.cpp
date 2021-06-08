#include "button_interrupt.h"
#include "roxie_config.h"

#define DEBOUNCE_DELAY 25
#define LONG_PRESS 2000

Button::Button(uint8_t button_pin)
{
	_button_pin = button_pin;
	pinMode(button_pin, INPUT_PULLUP);
}

bool Button::update_button()
{
	if (_risen_flag)
	{
		if (_fallen_flag == true)
		{
			if (_risen_time - _fallen_time < DEBOUNCE_DELAY)
			{
				DEB("But->No action:  " + String(_risen_time - _fallen_time) + " ms");
			}
			else
			{
				DEB("But->Press:  " + String(_risen_time - _fallen_time) + " ms");
				_clicked = true;
			}
			_fallen_flag = false;
		}
		_risen_flag = false;
	}

	if (_fallen_flag == true)
	{
		if ((millis() - _fallen_time) > LONG_PRESS)
		{
			DEB("But->Long press:  " + String(millis() - _fallen_time) + " ms");
			_long_click = true;
			_fallen_flag = false;
			return true;
		}
	}
	return false;
}

void Button::button_changed()
{
	if (digitalRead(_button_pin) == LOW)
	{
		_fallen_time = millis();
		_fallen_flag = true;
		DEB("low");
	}
	else
	{
		_risen_flag = true;
		_risen_time = millis();
		DEB("high");
	}
}

bool Button::get_clicked()
{
	bool clicked = _clicked;
	_clicked = false;
	return clicked;
}

bool Button::get_long_click()
{
	bool long_click = _long_click;
	_long_click = false;
	return long_click;
}