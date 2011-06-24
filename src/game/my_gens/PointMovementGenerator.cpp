/*
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "PointMovementGenerator.h"
#include "Errors.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "TemporarySummon.h"
#include "World.h"
#include "Movement/UnitMovement.h"

extern void GeneratePath(const WorldObject*, G3D::Vector3, const G3D::Vector3&, std::vector<G3D::Vector3>&, bool isFlight = false);

//----- Point Movement Generator
template<class T>
void PointMovementGenerator<T>::Initialize(T &unit)
{
    unit.addUnitState(UNIT_STAT_ROAMING|UNIT_STAT_ROAMING_MOVE);

    arrived = false;
    using namespace Movement;

    UnitMovement& state = *unit.movement;
    MoveCommonInit init(state);
    GeneratePath(&unit,state.GetPosition3(),Vector3(i_x,i_y,i_z), init.Path(), state.IsFlying());
    init.Launch();
    mySpline = state.MoveSplineId();
}

template<class T>
void PointMovementGenerator<T>::Finalize(T &unit)
{
    unit.clearUnitState(UNIT_STAT_ROAMING|UNIT_STAT_ROAMING_MOVE);
}

template<class T>
void PointMovementGenerator<T>::Interrupt(T &unit)
{
    unit.clearUnitState(UNIT_STAT_ROAMING|UNIT_STAT_ROAMING_MOVE);
}

template<class T>
void PointMovementGenerator<T>::Reset(T &unit)
{
    if (!unit.IsStopped())
        unit.StopMoving();

    unit.addUnitState(UNIT_STAT_ROAMING|UNIT_STAT_ROAMING_MOVE);
}

template<class T>
bool PointMovementGenerator<T>::Update(T &unit, const uint32 &diff)
{
    if(unit.hasUnitState(UNIT_STAT_CAN_NOT_MOVE))
    {
        unit.clearUnitState(UNIT_STAT_ROAMING_MOVE);
        return true;
    }

    unit.addUnitState(UNIT_STAT_ROAMING_MOVE);
    return !arrived;
}

template<class T>
void PointMovementGenerator<T>::OnEvent(Unit& unit, const Movement::OnEventArgs& args)
{
    if (mySpline != args.splineId)
    {
        return;
    }

    if (args.isArrived())
    {
        arrived = true;
        unit.clearUnitState(UNIT_STAT_MOVING);
        if (unit.GetTypeId() == TYPEID_UNIT)
            MovementInform((Creature&)unit);
    }
}

template <class T>
void PointMovementGenerator<T>::MovementInform(Creature &unit)
{
    if (unit.AI())
        unit.AI()->MovementInform(POINT_MOTION_TYPE, id);

    if (unit.IsTemporarySummon())
    {
        TemporarySummon* pSummon = (TemporarySummon*)(&unit);
        if (pSummon->GetSummonerGuid().IsCreature())
            if(Creature* pSummoner = unit.GetMap()->GetCreature(pSummon->GetSummonerGuid()))
                if (pSummoner->AI())
                    pSummoner->AI()->SummonedMovementInform(&unit, POINT_MOTION_TYPE, id);
    }
}

template void PointMovementGenerator<Player>::Initialize(Player&);
template void PointMovementGenerator<Creature>::Initialize(Creature&);
template void PointMovementGenerator<Player>::Finalize(Player&);
template void PointMovementGenerator<Creature>::Finalize(Creature&);
template void PointMovementGenerator<Player>::Interrupt(Player&);
template void PointMovementGenerator<Creature>::Interrupt(Creature&);
template void PointMovementGenerator<Player>::Reset(Player&);
template void PointMovementGenerator<Creature>::Reset(Creature&);
template bool PointMovementGenerator<Player>::Update(Player &, const uint32 &diff);
template bool PointMovementGenerator<Creature>::Update(Creature&, const uint32 &diff);

void AssistanceMovementGenerator::Finalize(Unit &unit)
{
    unit.clearUnitState(UNIT_STAT_ROAMING|UNIT_STAT_ROAMING_MOVE);

    ((Creature*)&unit)->SetNoCallAssistance(false);
    ((Creature*)&unit)->CallAssistance();
    if (unit.isAlive())
        unit.GetMotionMaster()->MoveSeekAssistanceDistract(sWorld.getConfig(CONFIG_UINT32_CREATURE_FAMILY_ASSISTANCE_DELAY));
}
