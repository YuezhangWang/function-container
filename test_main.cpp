#include <iostream>
#include <string>
#include "include/FuntionContainer.hpp"
using namespace std;

int Main(const vector<string>& args);

int main(int argc, char* argv[])
{
	vector<string> args(static_cast<size_t>(argc));
	for (size_t i = 0; i < argc; i++)
	{
		args[i] = argv[i];
	}
	return Main(args);
}

template <typename Fn, typename... Args>
inline auto Wrapper(Fn&& fn, Args&&... args) -> decltype(fn(forward<Args>(args)...))
{
	return fn(forward<Args>(args)...);
}

/// <summary>
/// Entry point
/// </summary>
/// <param name="args">arguments passed by command line</param>
/// <returns></returns>
int Main(const vector<string>& args)
{
	Util::FunctionContainer con;
	try
	{
		con.RegisterHandler("Add", [](int a, int b) { return a + b; });
		con.RegisterHandler("Void", []() { cout << "this is a void test." << endl; });
		con.RegisterHandler("String", [](int a, const string& b) { return to_string(a) + b; });
		con.RegisterHandler("Ptr", [](const char* data, int size) { cout << data << " " << size << endl; });

		auto r = con.Call<int>("Add", 1, 2);
		cout << r << endl;
		auto s = con.Call<string>("String", 142857, string("test"));
		cout << s << endl;
		con.VoidCall("Void");
		con.VoidCall("Ptr", "test", 66);
		con.VoidCall("Non-exist"); // throw invalid_argument exception
	}
	catch (invalid_argument e)
	{
		cerr << "Exception:\n" << e.what() << endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}