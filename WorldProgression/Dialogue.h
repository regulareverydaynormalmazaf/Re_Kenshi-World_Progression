// Minimal declaration so KenshiLib resolves Dialogue::_checkCondition by symbol
// (Steam + GOG). Mangles to:
//   ?_checkCondition@Dialogue@@QEAA_NW4DialogConditionEnum@@W4ComparisonEnum@@HPEAVCharacter@@2@Z
// KenshiLib does not ship a Dialogue header, so we declare just what we hook.
#pragma once
#include <kenshi/Enums.h>   // DialogConditionEnum, ComparisonEnum (real values from KenshiLib)
class Character;
class Dialogue {
public:
    bool _checkCondition(DialogConditionEnum conditionName, ComparisonEnum compareBy, int val,
                         Character* target, Character* actualConversationTarget);
};
