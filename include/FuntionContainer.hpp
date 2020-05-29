#pragma once
#include <string>
#include <functional>
#include <type_traits>
#include <map>
#include <tuple>
#include <exception>
#include <stdexcept>

namespace Util::detail
{
	template <typename T>
	class function_traits;

	// normal function
	template <typename Ret, typename... Args>
	class function_traits<Ret(Args...)>
	{
	public:
		enum { ARITY = sizeof...(Args) };
		typedef Ret function_type(Args...);
		typedef Ret result_type;
		using stl_function_type = std::function<function_type>;
		typedef Ret(*pointer)(Args...);

		template <size_t I>
		class args
		{
			static_assert(I < ARITY, "index is out of range, index must less than size of \"Args\"");
			using type = typename std::tuple_element<I, std::tuple<Args...>>::type;
		};

		typedef std::tuple<std::remove_cv_t<std::remove_reference_t<Args>>...> tuple_type;
		typedef std::tuple<std::remove_const_t<std::remove_reference_t<Args>>...> bare_tuple_type;
	};

	// pointer to function
	template <typename Ret, typename... Args>
	class function_traits<Ret(*)(Args...)> : public function_traits<Ret(Args...)> {};

	// std::function
	template <typename Ret, typename... Args>
	class function_traits<std::function<Ret(Args...)>> : public function_traits<Ret(Args...)> {};

	// member function.
#define FUNCTION_TRAITS(...)\
	template <typename ReturnType, typename ClassType, typename... Args>\
	class function_traits<ReturnType(ClassType::*)(Args...) __VA_ARGS__> : public function_traits<ReturnType(Args...)>{};\

	FUNCTION_TRAITS()
		FUNCTION_TRAITS(const)
		FUNCTION_TRAITS(volatile)
		FUNCTION_TRAITS(const volatile)

#undef FUNCTION_TRAITS

		// function object
		template <typename Callable>
	class function_traits : public function_traits<decltype(&Callable::operator())> {};

	template <typename Function>
	typename function_traits<Function>::stl_function_type to_function(const Function& lambda)
	{
		return static_cast<typename function_traits<Function>::stl_function_type>(lambda);
	}

	template <typename Function>
	typename function_traits<Function>::stl_function_type to_function(Function&& lambda)
	{
		return static_cast<typename function_traits<Function>::stl_function_type>(std::forward<Function>(lambda));
	}

	template <typename Function>
	typename function_traits<Function>::pointer to_function_pointer(const Function& lambda)
	{
		return static_cast<typename function_traits<Function>::pointer>(lambda);
	}

	class FunctionContainer
	{
	public:
		/// <summary>
		/// Register lambda expression to container itself
		/// </summary>
		/// <typeparam name="Function"></typeparam>
		/// <param name="name">Unique function name as a key to access your function registerd</param>
		/// <param name="fn">function object to be registerd</param>
		template <typename Function>
		void RegisterHandler(std::string const& name, const Function& fn)
		{
			using std::placeholders::_1;
			using std::placeholders::_2;

			using return_type = typename function_traits<Function>::result_type;

			this->invokers[name] = { std::bind(&Invoker<Function>::Apply, fn, _1, _2) };
		}

		/// <summary>
		/// Call the function you registered before
		/// </summary>
		/// <typeparam name="T"></typeparam>
		/// <typeparam name="...Args"></typeparam>
		/// <param name="name">Unique function name</param>
		/// <param name="...args">the arguments to be passed</param>
		/// <returns></returns>
		template <typename T, typename... Args>
		T Call(const std::string& name, Args&&... args)
		{
			auto it = invokers.find(name);
			if (it == invokers.end())
			{
				//return {};
				std::string message;
				message = "Function: \"" + name + "\"" + " has not been registered.";
				throw std::invalid_argument(message);
			}

			auto args_tuple = std::make_tuple(std::forward<Args>(args)...);

			char data[sizeof(std::tuple<Args...>)];
			std::tuple<Args...>* tp = new (data) std::tuple<Args...>;
			*tp = args_tuple;

			T t;
			it->second(tp, &t);
			return t;
		}

		/// <summary>
		/// Call the void function you registered before
		/// </summary>
		/// <typeparam name="...Args"></typeparam>
		/// <param name="name">Unique function name</param>
		/// <param name="...args">the arguments to be passed</param>
		template <typename... Args>
		void VoidCall(const std::string& name, Args&&... args)
		{
			auto it = invokers.find(name);
			if (it == invokers.end())
			{
				//return;
				std::string message;
				message = "Function: \"" + name + "\"" + " has not been registered.";
				throw std::invalid_argument(message);
			}

			auto args_tuple = std::make_tuple(std::forward<Args>(args)...);
			it->second(&args_tuple, nullptr);
		}

	private:
		template <typename Function>
		class Invoker
		{
		public:
			static inline void Apply(const Function& func, void* bl, void* result)
			{
				using tuple_type = typename function_traits<Function>::tuple_type;
				const tuple_type* tp = static_cast<tuple_type*>(bl);

				Call(func, *tp, result);
			}

		private:
			template <typename F, size_t... I, typename... Args>
			static auto CallHelper(const F& f, const std::index_sequence<I...>&, const std::tuple<Args...>& tup)
			{
				return f(std::get<I>(tup)...);
			}

			template <typename F, typename... Args>
			static typename std::enable_if<std::is_void<typename std::result_of<F(Args...)>::type>::value>::type
				Call(const F& f, const std::tuple<Args...>& tp, void*)
			{
				CallHelper(f, std::make_index_sequence<sizeof...(Args)>{}, tp);
			}

			template <typename F, typename... Args>
			static typename std::enable_if<!std::is_void<typename std::result_of<F(Args...)>::type>::value>::type
				Call(const F& f, const std::tuple<Args...>& tp, void* result)
			{
				auto r = CallHelper(f, std::make_index_sequence<sizeof...(Args)>{}, tp);
				*(decltype(r)*)result = r;
			}
		};

		std::map<std::string, std::function<void(void*, void*)>> invokers;
	};
}

namespace Util
{
	/// <summary>
	/// Generic function container
	/// </summary>
	using FunctionContainer = detail::FunctionContainer;
}