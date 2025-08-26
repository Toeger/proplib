# prop::Screen_unit\<Screen_unit_type>

## Rational
Sizes are convenient to specify in different unity.
Sometimes you want a 12 point font,
sometimes a 7mm sized button so it can be clicked or pressed easily
and sometimes you want an element to take up 70% of the screen width.

To facilitate this, Proplib provides a variety of units:

| Type | UDL | Description |
| --- | --- | --- |
| `prop::Pixels` | `_px` | Screen pixels, the default unit |
| `prop::X_millimeters` | `_xmm` | Horizontal millimeters |
| `prop::Y_millimeters` | `_ymm` | Vertical millimeters |
| `prop::Points` | `_pt` | DTP points, typically used with fonts |
| `prop::Screen_width_percents` | `_swp` | Screen width / 100 |
| `prop::Screen_height_percents` | `_shp` | Screen height / 100 |
| `prop::Screen_size_percents` | `_ssp` | Screen width * screen height / 100 / 100 |

The UDL (User-Defined Literals) require buy-in via `using prop::literals;` before they are available to avoid naming collisions.

Pixels tend to not be perfectly square, so for perfectionists there are both `_xmm` and `_ymm`.
People who don't care can add `_mm`, see section Adding UDLs below.

### Example
```cpp
#include <prop/ui/button.h>

prop::Button button;
button.width = prop::Pixels{100};
using prop::literals;
button.height = 12_pt;
```

### Adding UDLs

You can easily add your own literals by specifying how they would convert to one of the above units.

```cpp
[[nodiscard]] constexpr inline auto operator""_in(long double factor) {
	return 25.4_xmm * factor;
}
[[nodiscard]] constexpr inline auto operator""_in(unsigned long long int factor) {
	return 25.4_xmm * factor;
}
```
Here we add `_in` for inches. An inch is 25.4mm, so we return 25.4 millimeters times the factor.
From then on you can write `10_in` anywhere a size or position is needed in Proplib.
Other conversions, such as to pixels, are handled by `prop::X_millimeters`.

### Printing
Screen units can be printed using `std::ostream`-based printing such as `std::cout`
and `std::formatter`-based printing such as `std::print`.
In either case the same format specifiers as for `PROP_SCREEN_UNIT_PRECISION`
(default `std::float_t`) can be used to customize the printing,
such as `std::setprecision(10)` or `{:10f}` respectively.

The UDL name (without the underscore) will be printed along the value, so `std::format("{}", 1.5_mm)` will be evaluate to `"1.5mm"`.
If the unit is not desired, you can use `.amount` to get the raw scalar value.
`std::cout << (42_xmm).amount;` will print `42`.

### Internal Representation
There seems to be no clear consensus on whether to prefer `float` or `double` when precision does not matter.
Some people insist `float` is faster because it is smaller
while others insist `double` is faster because any `float` operation is actually a `double` operation,
just with additional conversions from and to `float`.
Proplib defaults to `std::float_t` which should default to the fastest type available that can represent `float` values.
Depending on platform, that may be `float` or `double` or theoretically `long double`.
Common platforms and compilers seem to use `float`.

This behavior can be overridden by defining `PROP_SCREEN_UNIT_PRECISION`,
such as `-DPROP_SCREEN_UNIT_PRECISION=double` in Cmake or compiler flags.
The type is assumed to be a floating point type,
`PROP_SCREEN_UNIT_PRECISION=int` is not compatible with Proplib.

# prop::Screen_dimension\<prop::Screen_dimension_type>

## Rational
In GUI code it is common to specify numbers of different dimensions.
For example, `prop::Widget`s have an X-position, a Y-position, a width and a height.
While often these are specified with type `int`, that allows nonsensical code
such as `Xpos * width`.
Only a subset of the arithmetic operations available for `int` make sense for these dimensions,
which the `prop::Screen_dimension` type reflects.

The following dimensions exist:

| Type |
| --- |
| `prop::Width` |
| `prop::Height` |
| `prop::Xpos` |
| `prop::Ypos` |
Additionally the scalar type (`PROP_SCREEN_UNIT_PRECISION`, default `std::float_t`) is used in some operations.

### Examples of allowed operations:

`Xpos + Width -> Xpos`

`Ypos - Ypos -> Height`

`Width * Scalar -> Width`

`Xpos / Xpos -> Scalar`

### Examples of disallowed operations:

`Xpos + Xpos`

`Width + Scalar`

`Width / Height`

`Width + Ypos`

`Xpos * Xpos`

In case an operation is not allowed, such as `Width / Height` to obtain the aspect ratio,
`.amount` can be used to obtain the raw `prop::Screen_unit` values.
