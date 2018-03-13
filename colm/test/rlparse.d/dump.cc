#include "parsedata.h"
#include "parsetree.h"

#include <iostream>
using std::cout;
using std::endl;

void ParseData::dump()
{
	cout << "machine name: " << sectionName << endl;

	for ( GraphDict::Iter g = graphDict; g.lte(); g++ )
		g->value->dump();
}

void VarDef::dump()
{
	cout << name << " = ";
	machineDef->dump();
	cout << endl;
}

void MachineDef::dump()
{
	switch ( type ) {
		case JoinType:
			join->dump();
			break;
		case LongestMatchType:
			longestMatch->dump();
			break;
		case LengthDefType:
			cout << "length def";
			break;
		case NfaUnionType:
			cout << "nfa union ";
			break;
	}
}

void LongestMatch::dump()
{
	cout << "|*";
	for ( LmPartList::Iter lmi = *longestMatchList; lmi.lte(); lmi++ ) {
		cout << " ";
		lmi->join->dump();
		cout << ";";
	}

	cout << " *|";
}

void Join::dump()
{
	for ( ExprList::Iter e = exprList; e.lte(); e++ )
		e->dump();
}

void Expression::dump()
{
	switch ( type ) {
		case OrType:
			expression->dump();
			cout << " | ";
			term->dump();
			break;
		case IntersectType:
			expression->dump();
			cout << " & ";
			term->dump();
			break;
		case SubtractType:
			expression->dump();
			cout << " - ";
			term->dump();
			break;
		case StrongSubtractType:
			expression->dump();
			cout << " -- ";
			term->dump();
			break;
		case TermType:
			term->dump();
			break;
		case BuiltinType:
			cout << "builtin";
			break;
	}
}

void Term::dump()
{
	switch ( type ) {
		case ConcatType:
			term->dump();
			cout << " . ";
			factorWithAug->dump();
			break;
		case RightStartType:
			term->dump();
			cout << " :> ";
			factorWithAug->dump();
			break;
		case RightFinishType:
			term->dump();
			cout << " :>> ";
			factorWithAug->dump();
			break;
		case LeftType:
			term->dump();
			cout << " <: ";
			factorWithAug->dump();
			break;
		case FactorWithAugType:
			factorWithAug->dump();
			break;
	}
}

void FactorWithAug::dump()
{
	factorWithRep->dump();
}

void FactorWithRep::dump()
{
	switch ( type ) {
		case StarType:
			factorWithRep->dump();
			cout << "*";
			break;
		case StarStarType:
			factorWithRep->dump();
			cout << "**";
			break;
		case OptionalType:
			factorWithRep->dump();
			cout << "?";
			break;
		case PlusType:
			factorWithRep->dump();
			cout << "+";
			break;
		case ExactType:
			factorWithRep->dump();
			cout << "{" << lowerRep << "}";
			break;
		case MaxType:
			factorWithRep->dump();
			cout << "{," << upperRep << "}";
			break;
		case MinType:
			factorWithRep->dump();
			cout << "{" << lowerRep << ",}";
			break;
		case RangeType:
			factorWithRep->dump();
			cout << "{" << lowerRep << "," << upperRep << "}";
			break;
		case FactorWithNegType:
			factorWithNeg->dump();
			break;
	}
}

void FactorWithNeg::dump()
{
	switch ( type ) {
		case NegateType:
			cout << "!";
			factorWithNeg->dump();
			break;
		case CharNegateType:
			cout << "^";
			factorWithNeg->dump();
			break;
		case FactorType:
			factor->dump();
			break;
	}
}

void Factor::dump()
{
	switch ( type ) {
		case LiteralType:
			cout << "lit";
			break;
		case RangeType:
			cout << "range";
			break;
		case OrExprType:
			cout << "or_expr";
			break;
		case RegExprType:
			cout << "reg_expr";
			break;
		case ReferenceType:
			cout << "ref";
			break;
		case ParenType:
			cout << "( ";
			join->dump();
			cout << " )";
			break;
		case LongestMatchType:
			cout << "F";
			break;
		case NfaRep:
			cout << "F";
			break;
		case CondStar:
			cout << "F";
			break;
		case CondPlus:
			cout << "F";
			break;
	}
}

