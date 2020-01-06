#include "ext\tinyxml2\tinyxml2.h"

#include <stdio.h>
#include <unordered_map>
#include <string>

using namespace tinyxml2;
using std::unordered_map;
using std::string;
using std::vector;

namespace {
	const char* Plus(int val)
	{
		return val >= 0 ? "+" : "";
	}

	// remove leading spaces and tabs
	// remove trailing spaces and tabs
	// replace newline and tabs with spaces
	// remove duplicated spaces
	string Clean(const char* s)
	{
		string c;
		if (s != nullptr)
		{
			bool leading = true;
			bool space = false;

			while (*s)
			{
				if (leading)
				{
					if (!(*s == ' ' || *s == '\n' || *s == '\t'))
					{
						c.append(1, *s);
						leading = false;
					}
				}
				else
				{
					if (*s == ' ' || *s == '\n' || *s == '\t')
					{
						if (!space)
						{
							c.append(1, ' ');
						}
						space = true;
					}
					else
					{
						c.append(1, *s);
						space = false;
					}
				}

				++s;
			}

			size_t size = c.size();
			while (size > 0)
			{
				char cha = c[size - 1];
				if (cha == ' ' || cha == '\n' || cha == '\t')
				{
					--size;
				}
				else
				{
					c.resize(size);
					break;
				}
			}
		}

		return c;
	}

	string FindShortDescription(tinyxml2::XMLElement* rule, const string& def)
	{
		string shortdesc;
		auto child = rule->FirstChildElement();
		while (child)
		{
			const char* name;
			child->QueryStringAttribute("name", &name);
			if (string(name) == "Short Description")
			{
				shortdesc = Clean(child->GetText());
				break;
			}

			child = child->NextSiblingElement();
		}

		if (shortdesc.empty())
		{
			const int shortlimit = 60;
			if (def.size() <= shortlimit+3)
			{
				shortdesc = def;
			}
			else
			{
				shortdesc = def.substr(0, shortlimit) + "...";
			}
		}

		return shortdesc;
	}
}

struct Trait
{
	string name;
	string shortdesc;
	string longdesc;
};

struct Feature
{
	string name;
	string shortdesc;
	string longdesc;
};

struct Feat
{
	string name;
	string shortdesc;
	string longdesc;
};

struct Weapon
{
	string name;
	int attackbonus = 0;
	string damage;
	string damagetype;
	string attackstat;
	string defense;
	string critdamage;
};

struct PowerLine
{
	string name;
	string info;
};

struct Power
{
	string name;
	string usage;
	string actiontype;
	string attacktype;
	string target;
	string flavor;
	string display;

	int level = 0;
	string powertype;

	std::vector<PowerLine> lines;
	std::vector<Weapon> weapons;
};

struct Character
{
	int AP = 1;
	int tempHP = 0;
	int death = 0;

	string name;
	string background;
	string theme;
	string gender;
	string race;
	string vision;
	string dndclass;
	string deity;
	string alignment;
	unordered_map<string, int> stats;
	vector<string> resist;
	vector<Trait> traits;
	vector<Feature> features;
	vector<Feat> feats;
	vector<Power> powers;

	void AddPower(const tinyxml2::XMLElement* pow)
	{
		Power power;

		const char* powername;
		pow->QueryStringAttribute("name", &powername);
		power.name = powername;

		auto child = pow->FirstChildElement();
		while (child != nullptr)
		{
			const char* t;
			string tag = child->Name();
			child->QueryStringAttribute("name", &t);
			string info(t);
			string text = Clean(child->GetText());

			if (tag == "specific")
			{
				if (info.empty() || info[0]=='_' || text.empty() || text.substr(0, 3) == "ID_")
				{
					// do nothing
				}
				else if (info == "Power Usage")
				{
					power.usage = text;
				}
				else if (info == "Action Type")
				{
					power.actiontype = text;
				}
				else if (info == "Attack Type")
				{
					power.attacktype = text;
				}
				else if (info == "Target")
				{
					power.target = text;
				}
				else if (info == "Flavor")
				{
					power.flavor = text;
				}
				else if (info == "Display")
				{
					power.display = text;
				}
				else if (info == "Level")
				{
					power.level = child->IntText();
				}
				else if (info == "Power Type")
				{
					power.powertype = text;
				}
				else
				{
					power.lines.push_back({ info, text });
				}
			}
			else if (tag == "Weapon")
			{
				Weapon weapon;
				weapon.name = info;

				auto weaponchild = child->FirstChildElement();
				while (weaponchild != nullptr)
				{
					string weapontag = weaponchild->Name();
					string weapontext = Clean(weaponchild->GetText());

					if (weapontag == "AttackBonus")
					{
						weapon.attackbonus = weaponchild->IntText();
					}
					else if (weapontag == "Damage")
					{
						weapon.damage = weapontext;
					}
					else if (weapontag == "DamageType")
					{
						weapon.damagetype = weapontext;
					}
					else if (weapontag == "AttackStat")
					{
						weapon.attackstat = weapontext;
					}
					else if (weapontag == "Defense")
					{
						weapon.defense = weapontext;
					}
					else if (weapontag == "CritDamage")
					{
						weapon.critdamage = weapontext;
					}

					weaponchild = weaponchild->NextSiblingElement();
				}

				power.weapons.push_back(weapon);
			}

			child = child->NextSiblingElement();
		}

		powers.push_back(power);
	}

	bool Load(const char* fn)
	{
		bool ret = false;

		XMLDocument doc;

		if (doc.LoadFile(fn) == XML_SUCCESS)
		{
			ret = true;
			auto root = doc.FirstChildElement("D20Character");
			auto sheet = root->FirstChildElement("CharacterSheet");
			auto details = sheet->FirstChildElement("Details");

			name = Clean(details->FirstChildElement("name")->GetText());

			auto statblock = sheet->FirstChildElement("StatBlock");
			auto stat = statblock->FirstChildElement();
			while (stat != nullptr)
			{
				int value;

				stat->QueryIntAttribute("value", &value);

				auto child = stat->FirstChildElement();
				while (child != nullptr)
				{
					auto attrib = child->FindAttribute("name");
					if (attrib != nullptr)
					{
						string attribname = attrib->Value();

						// fix con
						if (attribname == "con")
						{
							attribname = "CON";
						}

						// check for resist:xxxxxxx and store the resistance name xxxxxxx
						if (attribname.substr(0, 7) == "resist:")
						{
							resist.push_back(attribname.substr(7));
						}

						stats[attribname] = value;
					}

					child = child->NextSiblingElement();
				}

				stat = stat->NextSiblingElement();
			}

			auto rulestally = sheet->FirstChildElement("RulesElementTally");
			auto rule = rulestally->FirstChildElement();
			while (rule != nullptr)
			{
				const char* rulename;
				const char* t;
				rule->QueryStringAttribute("name", &rulename);
				rule->QueryStringAttribute("type", &t);

				string ruletype(t);
				if (ruletype == "Gender")
				{
					gender = rulename;
				}
				else if (ruletype == "Background")
				{
					background = rulename;
				}
				else if (ruletype == "Theme")
				{
					theme = rulename;
				}
				else if (ruletype == "Race")
				{
					race = rulename;
				}
				else if (ruletype == "Vision")
				{
					vision = rulename;
				}
				else if (ruletype == "Racial Trait")
				{
					string longdesc = Clean(rule->LastChild()->Value());
					string shortdesc = FindShortDescription(rule, longdesc);
					traits.push_back({ rulename, shortdesc, longdesc });
				}
				else if (ruletype == "Class")
				{
					dndclass = rulename;
				}
				else if (ruletype == "Deity")
				{
					deity = rulename;
				}
				else if (ruletype == "Alignment")
				{
					alignment = rulename;
				}
				else if (ruletype == "Class Feature")
				{
					string longdesc = Clean(rule->LastChild()->Value());
					string shortdesc = FindShortDescription(rule, longdesc);
					features.push_back({ rulename, shortdesc, longdesc });
				}
				else if (ruletype == "Feat")
				{
					string longdesc = Clean(rule->LastChild()->Value());
					string shortdesc = FindShortDescription(rule, longdesc);
					feats.push_back({ rulename, shortdesc, longdesc });
				}

				rule = rule->NextSiblingElement();
			}

			auto powerstats = sheet->FirstChildElement("PowerStats");
			auto pow = powerstats->FirstChildElement();
			while (pow != nullptr)
			{
				AddPower(pow);
				pow = pow->NextSiblingElement();
			}
		}

		return ret;
	}
};

namespace {
	void PrintAbility(Character& C, const string& aname, const string& longname)
	{
		int val = C.stats[aname];
		int mod = C.stats[longname + " modifier"];
		printf("%s: %d %s%d\n", aname.c_str(), val, Plus(mod), mod);
	}

	void PrintSkill(Character& C, const string& sname)
	{
		int val = C.stats[sname];
		bool trained = C.stats[sname + " Trained"];
		printf("%s: %s%d %s\n", sname.c_str(), Plus(val), val, trained ? "trained" : "");
	}

	void PrintStat(Character& C, const string& sname)
	{
		printf("%s: %d\n", sname.c_str(), C.stats[sname]);
	}
}

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		printf("Usage: CharSheetConsole char.dnd4e\n");
	}
	else
	{
		Character C;
		if (C.Load(argv[1]))
		{
			printf("Name: %s\n", C.name.c_str());
			printf("Class: %s\n", C.dndclass.c_str());
			printf("Background: %s\n", C.background.c_str());
			printf("Theme: %s\n", C.theme.c_str());
			printf("Race: %s\n", C.race.c_str());
			printf("Gender: %s\n", C.gender.c_str());
			printf("Deity: %s\n", C.deity.c_str());
			printf("Alignment: %s\n", C.alignment.c_str());
			PrintStat(C, "Level");
			int init = C.stats["Initiative"];
			printf("Initiative: %s%d\n", Plus(init), init);
			PrintStat(C, "Speed");
			printf("Action Points: %d\n", C.AP);

			printf("\nDefenses\n");
			PrintStat(C, "AC");
			PrintStat(C, "Fortitude");
			PrintStat(C, "Reflex");
			PrintStat(C, "Will");

			printf("\nHealth\n");
			int hp = C.stats["Hit Points"];
			printf("HP: %d\n", hp);
			printf("Temp HP: %d\n", C.tempHP);
			printf("Healing Surge Value: %d\n", hp / 4);
			PrintStat(C, "Healing Surges");
			printf("Failed Death Saves: %d\n", C.death);

			printf("\nAbilities\n");
			PrintAbility(C, "STR", "Strength");
			PrintAbility(C, "CON", "Constitution");
			PrintAbility(C, "DEX", "Dexterity");
			PrintAbility(C, "INT", "Intelligence");
			PrintAbility(C, "WIS", "Wisdom");
			PrintAbility(C, "CHA", "Charisma");

			printf("\nResistances\n");
			if (C.resist.size() > 0)
			{
				for (const auto& res : C.resist)
				{
					printf("%s: %d\n", res.c_str(), C.stats["resist:" + res]);
				}
			}
			else
			{
				printf("None\n");
			}

			printf("\nSkills\n");
			PrintSkill(C, "Acrobatics");
			PrintSkill(C, "Athletics");
			PrintSkill(C, "Arcana");
			PrintSkill(C, "Bluff");
			PrintSkill(C, "Diplomacy");
			PrintSkill(C, "Dungeoneering");
			PrintSkill(C, "Endurance");
			PrintSkill(C, "Heal");
			PrintSkill(C, "History");
			PrintSkill(C, "Insight");
			PrintSkill(C, "Intimidate");
			PrintSkill(C, "Nature");
			PrintSkill(C, "Perception");
			PrintSkill(C, "Religion");
			PrintSkill(C, "Stealth");
			PrintSkill(C, "Streetwise");
			PrintSkill(C, "Thievery");

			printf("\nSenses\n");
			PrintStat(C, "Passive Insight");
			PrintStat(C, "Passive Perception");
			printf("Vision: %s\n", C.vision.c_str());

			printf("\nClass Features\n");
			if (C.features.size() > 0)
			{
				for (const auto& feature : C.features)
				{
					printf("%s: %s\n", feature.name.c_str(), feature.shortdesc.c_str());
				}
			}
			else
			{
				printf("None\n");
			}

			printf("\nTraits\n");
			if (C.traits.size() > 0)
			{
				for (const auto& trait : C.traits)
				{
					printf("%s: %s\n", trait.name.c_str(), trait.shortdesc.c_str());
				}
			}
			else
			{
				printf("None\n");
			}

			printf("\nFeats\n");
			if (C.feats.size() > 0)
			{
				for (const auto& feat : C.feats)
				{
					printf("%s: %s\n", feat.name.c_str(), feat.shortdesc.c_str());
				}
			}
			else
			{
				printf("None\n");
			}

			printf("\nPowers\n");
			if (C.powers.size() > 0)
			{
				for (const auto& power : C.powers)
				{
					printf("%s\n", power.name.c_str());
					printf("%s <> %s\n", power.usage.c_str(), power.actiontype.c_str());
					printf("%s\n", power.attacktype.c_str());
					if (!power.target.empty())
					{
						printf("Target: %s\n", power.target.c_str());
					}
					printf("%s\n", power.flavor.c_str());
					for (const auto& line : power.lines)
					{
						printf("%s: %s\n", line.name.c_str(), line.info.c_str());
					}
					if (!power.weapons.empty())
					{
						const auto& w = power.weapons[0];
						string damagetype = w.damagetype.empty() ? "" : " " + w.damagetype;
						printf("%s: %s%d vs %s, %s%s; critical %s\n", 
							w.name.c_str(),
							Plus(w.attackbonus), w.attackbonus, 
							w.defense.c_str(), 
							w.damage.c_str(), 
							damagetype.c_str(), 
							w.critdamage.c_str());
					}
					printf("\n");
				}
			}
		}
		else
		{
			printf("Error: Unable to LoadFile(%s)\n", argv[1]);
		}
	}
}

