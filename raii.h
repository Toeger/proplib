#ifndef RAII_H
#define RAII_H

#include <utility>

namespace prop::detail {
	template <class Init_function, class Exit_function>
	struct RAII {
		RAII(Init_function init_function, Exit_function p_exit_function)
			: exit_function{std::move(p_exit_function)} {
			init_function();
		}
		RAII(Exit_function p_exit_function)
			: RAII(do_nothing, std::move(p_exit_function)) {}
		~RAII() {
			if (!canceled) {
				exit_function();
			}
		}
		void cancel() {
			canceled = true;
		}
		void early_exit() {
			exit_function();
			cancel();
		}

		private:
		static void do_nothing() {}
		Exit_function exit_function;
		bool canceled = false;
	};

	template <class Init_function, class Exit_function>
	RAII(Init_function, Exit_function) -> RAII<Init_function, Exit_function>;
	template <class Exit_function>
	RAII(Exit_function) -> RAII<void(), Exit_function>;
} // namespace prop::detail

#endif
