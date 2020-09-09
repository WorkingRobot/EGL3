#pragma once

struct RespGetExternalSourceSettings {
	// Whether to not ask the user to link their account (only seen false)
	bool DoNotShowLinkingProposal;

	PARSE_DEFINE(RespGetExternalSourceSettings)
		PARSE_ITEM("doNotShowLinkingProposal", DoNotShowLinkingProposal)
	PARSE_END
};