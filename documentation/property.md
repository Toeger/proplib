# prop::Property\<T>

## Rational

`prop::Property<T>` is a templated wrapper.
Its purpose is to detect and propagate changes.

Note: Properties exist as a language feature in C# and JavaScript.
However, those properties are effectively getters where the `()` are ommitted and have nothing to do with propagating changes and therefore are unrelated.
QML has properties as a language feature in the sense of propagating changes.

### Example

```cpp
prop::Property<int> pa = 42;
prop::Property<int> pb = [&pa] { return pa + 1; };
std::cout << pb; //43
pa = 2;
std::cout << pb; //3
```
This example demonstrates how properties enable a reactive style of programming.

`pa` is an `int` property and assigned the value `42`.  
`pb` is an `int` property that is assigned the function `pa + 1`.
Consequently its value is `43`.  
When `pa`'s value is changed to `2`, `pb`'s value updates accordingly to `2 + 1` which is `3`.

This updating mechanism is a core component of Proplib. It is used internally to express dependencies between GUI elements
(such as the positional logic of widgets in a layout) as well as for users of the library to express their application logic
(such as displaying backend data). Properties can be used independently of a GUI, though may be less useful there.

TODO: Use a more GUI-focused example that is actually useful.

## Wrapper Limitations

`prop::Property<T>` tries to behave like a `T`. However, due to the lack of `operator.` this is not completely possible.
Even simple code like
```cpp
prop::Property<std::string> ps = "Hello world";
std::cout << ps.size();
```
will fail to compile, because `prop::Property` does not have a member `.size` and is unable to forward the one from `std::string`.
As a workaround, `ps->size()` can be used which forwards correctly as `operator->` is overloadable, but it is different syntax than `std::string` uses.  
TODO: References

## Preference of `const`

Whenever a property is changed, those changes are propagated to depending properties.
Those depending properties might have more dependents and propagate changes further.
While this is the intended and often necessary behavior, it can also be computationally expensive.
To avoid unnecessary updates and not fall into infinite loops, Proplib tries to avoid unnecessary update notifications.
The way it does so is by trying to pass a `const` version of the wrapped type `T` whenever it is accessed.
Example:
```cpp
prop::Property<std::string> ps = "Hello";
ps->data()[0] = 'B'; //error, cannot assign to const char &
```
If `ps` was an `std::string`, `ps.data()[0] = 'B';` would compile (since C++17) because `.data()` returns a `char *`.
However, `prop::Property` will instead prefer the `.data()` from a `const std::string` which also compiles, but returns a `const char *`.
That way, `prop::Property` knows there will be no modification of the `std::string`, so it does not need to run update functions.

While this behavior avoids unnecessary updates by default, sometimes you really do want to make a modification.
This can be done using `ps.apply()->data()[0] = 'B';`.

## Batching Modifications

TODO: guarded

## Function Forms

As seen in the example, it is possible to assign functions to properties that are called automatically when a dependency changes. 
There are two forms such update functions can take, return functions and value functions.

### Return Functions

A return function is a function that returns the underlying type or something convertible to it.
For example, the update function for a `prop::Property<double>` might return an `int`.
This is suitable for simple functions.
TODO: Example  
However, sometimes it is necessary for an update function to take the previous value into account, in which case the value function form should be used.

### Value Functions

A value function is a function that returns a `prop::Value` and takes a the underlying type as its first parameter (possibly via (`const`) `*`/`&`).
`prop::Value` is an enum with the following values:

| Value | Meaning |
| --- | --- |
| `prop::Value::changed` | The value has been changed and dependents of this property will be updated. |
| `prop::Value::unchanged` | The value has not been changed and dependents of this property will not be updated. |
| `prop::Value::sever` | The update function will be removed from this property. Dependents will be updated. |

TODO: Example

### Implicit Dependencies

Whenever a property reads another property in its update function, that other property becomes an implicit dependency.
Whenever such an implicit dependency changes, the update function is called again.
This may trigger further updates if there are dependents on the updated property.

### Severing

Severing means a property loses its update function.
This can be done explicitly using the `.sever()` function on a `prop::Property`, returning `prop::Value::sever` from a value function or implicitly when a dependency is destroyed.
Severing helps avoid memory errors by not running update functions that access a destroyed property.

### Explicit Dependencies

Dependencies can be specified explicitly using TODO: Syntax  
The type of the parameter does not have to match the explicit dependency exactly.
The following types can be used:

| Type | Causes Update | Causes Severing |
| --- | --- | --- |
| `T` | No | Yes |
| `const T` | No | Yes |
| `const T &` | No | Yes |
| `T &` | Yes | Yes |
| `T *` | Yes | No |
| `const T *` | No | No |
| `prop::Property<T>` | No | Yes |
| `const prop::Property<T>` | No | Yes |
| `const prop::Property<T> &` | No | Yes |
| `prop::Property<T> &` | Yes | Yes |
| `prop::Property<T> *` | Yes | No |
| `const prop::Property<T> *` | No | No |
| `std::is_constructible<U, T> and not std::is_constructible<U, std::nullptr_t>` | No | Yes |
| `std::is_constructible<U, T> and std::is_constructible<U, std::nullptr_t>` | No | `not std::is_constructible<U, std::nullptr_t>` |

"Causes Update" means that the source property is assumed to have changed, which triggers an update.
Whenever a `const T` is sufficient, no update will be caused.  
"Causes Severing" means that when the source property is destroyed, the property using it through the given type is severed.
Whenever `nullptr` can be passed, no severing will be caused.
In cases that don't cause severing, `nullptr` is passed. This allows for update functions that can tolerate sources to disappear.
It is on the update function to handle `nullptr` gracefully.  
Note that should `T` be for example `int *`, severing will still happen and `nullptr` only passed if the source property exists and contains `nullptr`.

### Prefer `const`

The parameter type of update functions does not have to be `const`. If it is, `const` can be removed and the update function will still work.
However, to avoid unnecessary work on updating properties that did not change it is recommended to `const`-qualify update function parameters whenever possible.

### No Ambiguous Functions

TODO: No overloading, no templating, no `auto`, no 

## Capturing `this`

GUI elements such as `prop::Button` are movable. Therefore the following code is problematic:
```cpp
class Locator : public prop::Label {
	Locator() {
		text = [this] {
			return std::format("x: {}\ny: {}", position.x, position.y);
		};
	}
};
```

The intention is to make a custom GUI element `Locator` that is a label that shows its current position.
At first this class is likely to work, but eventually it will cause crashes.
The reason for the crashes is that the lambda captures `this` which points to the `Locator` object.
This `Locator` object might be moved into a layout.
After the move, the captured `this` pointer will continue pointing to the old instance, causing incorrect behavior or a crash.

The solution is to capture a "selfie" instead.
```cpp
class Locator : public prop::Label {
	Locator() {
		text = {
			[] (Locator &self) {
				return std::format("x: {}\ny: {}", self->position.x, self->position.y);
			},
			selfie()
		};
	}
};
```
The `this` capture is replaced by `selfie()` which returns a property pointing to the widget (`Locator`, not `prop::Widget`).
This property will update whenever the widget is moved, thus keep the text reference the correct position.
TODO: sever on destruction

## Multithreading

Unfortunately, Proplib does not lend itself well to multithreading.
Properties are intended to be used in large dependency chains which means that changing one property may end up implicitly changing many of them.
While it is theoretically possible to use disconnected sets of properties from different threads, in practice this is very likely to cause undefined behavior in the form of data races.

TODO: Counter measures
