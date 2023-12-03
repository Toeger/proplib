#pragma once

#include "property.h"

#include <functional>

#if 0
namespace prop {
	class Binding : prop::detail::Property_base {
		public:
		Binding();
		Binding(std::function<void()> callback);
		Binding &operator=(std::function<void()> callback);
		template <class... Args>
		void bind(std::function<void(std::type_identity_t<const prop::Property<Args> *>...)> callback,
				  const prop::Property<Args> *...property_pointers);
		void unbind() override;
		void update() override final;

		private:
		void update_source(std::function<void()> f);
		std::function<void()> callback;

		template <class Functor, class... Args, std::size_t... Indexes>
		static void call_with_explicit_dependencies(Functor &f, std::tuple<Args...> *tuple,
													detail::Binding_list &explicit_dependencies,
													std::index_sequence<Indexes...>) {
			assert(std::size(explicit_dependencies) == sizeof...(Indexes));
			f(static_cast<std::remove_reference_t<decltype(std::get<Indexes>(*tuple))>>(
				explicit_dependencies[Indexes])...);
		}
	};
} // namespace prop

namespace prop {
	template <class... Args>
	void Binding::bind(std::function<void(std::type_identity_t<const prop::Property<Args> *>...)> callback,
					   const prop::Property<Args> *...property_pointers) {
		explicit_dependencies = {{const_cast<prop::Property<Args> *>(property_pointers)...}};
		update_source([this, callback = std::move(callback)] {
			call_with_explicit_dependencies(callback, (std::tuple<const Property<Args> *...> *){},
											explicit_dependencies, std::index_sequence_for<Args...>());
		});
	}
} // namespace prop
#endif
