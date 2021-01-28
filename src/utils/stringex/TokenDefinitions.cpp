#include "TokenDefinitions.h"

namespace EGL3::Utils::StringEx {
	TokenDefinitions::TokenDefinitions() :
		IgnoreWhitespace(false)
	{

	}

	bool TokenDefinitions::GetIgnoreWhitespace() const
	{
		return IgnoreWhitespace;
	}

	void TokenDefinitions::SetIgnoreWhitespace(bool Value)
	{
		IgnoreWhitespace = Value;
	}

	void TokenDefinitions::DefineToken(std::function<ExpressionError(TokenStream&, std::vector<ExpressionToken>&)>&& Definition)
	{
		Definitions.emplace_back(std::move(Definition));
	}

	ExpressionError TokenDefinitions::Lex(const std::string& Input, std::vector<ExpressionToken>& Output) const
	{
		Output.clear();
		TokenStream Stream(Input);

		while (!Stream.IsEmpty()) {
			auto Error = LexOne(Stream, Output);
			if (Error.HasError()) {
				return Error;
			}
		}

		return ExpressionError();
	}

	ExpressionError TokenDefinitions::LexOne(TokenStream& Stream, std::vector<ExpressionToken>& Output) const
	{
		if (IgnoreWhitespace) {
			auto Whitespace = Stream.ParseWhitespace();
			if (Whitespace) {
				Stream.Seek(Whitespace);
			}
		}

		if (Stream.IsEmpty()) {
			return ExpressionError();
		}

		size_t Position = Stream.Tell();

		for (auto& Definition : Definitions) {
			auto Error = Definition(Stream, Output);
			if (Error.HasError()) {
				return Error;
			}

			if (Stream.Tell() != Position) {
				return ExpressionError();
			}
		}

		return ExpressionError("Could not lex expression");
	}
}