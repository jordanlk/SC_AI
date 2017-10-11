#include "ProductionManager.h"

ProductionManager::ProductionManager()
{
	speedtest = 1;
	hello = "Hello World";
	ProductionManager::setGas(0);
	setReservedGas(0);
	ProductionManager::minerals = 0;
	reserved_minerals = 0;
	setSupply(4);
	getMaxSupply(9);
	supply_percentage = 0;
	hold_worker_production = false;

	if (!race)
		race = Broodwar->self()->getRace();

	minerals_status = NONE;
	gas_status = NONE;
	supply_status = NONE;
	/*

	*/

	//CANT USE THIS LINE OTHERWISE BREAKS THE DLL
	//if (setZealotRushQueues()) Broodwar << "QUEUE SETADA " << endl;
}

void ProductionManager::constructorInit()
{
	speedtest = 1;
	hello = "Hello World";
	ProductionManager::setGas(0);
	setReservedGas(0);
	ProductionManager::minerals = 0;
	reserved_minerals = 0;
	setSupply(4);
	getMaxSupply(9);
	supply_percentage = 0;
	hold_worker_production = false;

	if (!race)
		race = Broodwar->self()->getRace();

	minerals_status = NONE;
	gas_status = NONE;
	supply_status = NONE;
	/*

	*/

	if (setZealotRushQueues()) Broodwar << "QUEUE SETADA " << endl;
	else Broodwar << "QUEUE ERROR " << endl;
}

void ProductionManager::updateResources()
{
	if (Broodwar->self()->isDefeated() || Broodwar->self()->isVictorious()) return;

	//int val = Broodwar->self()->getRace()->getWorker()->getType().mineralPrice();
	//mineral sorting

	setMinerals(Broodwar->self()->minerals());
	setSupply(Broodwar->self()->supplyUsed());
	getMaxSupply(Broodwar->self()->supplyTotal());
	setGas(Broodwar->self()->gas());

	{
		if (getMinerals() == 0)
		{
			setMineralStatus(NONE);
		}
		else if (getMinerals() > 0 && getMinerals() <= 400)
		{
			setMineralStatus(LITTLE);
		}
		else if (getMinerals() > 400 && getMinerals() <= 1000)
		{
			setMineralStatus(SOME);
		}
		else if (getMinerals() > 1000 && getMinerals() <= 2500)
		{
			setMineralStatus(PLENTY);
		}
		else if (getMinerals() > 2500 && getMinerals() <= 70000)
		{
			setMineralStatus(FULL);
		}
	}

	//gas sorting
	{
		if (getGas() == 0)
		{
			setGasStatus(NONE);
		}
		else if (getGas() > 0 && getGas() <= 400)
		{
			setGasStatus(LITTLE);
		}
		else if (getGas() > 400 && getGas() <= 1000)
		{
			setGasStatus(SOME);
		}
		else if (getGas() > 1000 && getGas() <= 2500)
		{
			setGasStatus(PLENTY);
		}
		else if (getGas() > 2500 && getGas() <= 70000)
		{
			setGasStatus(FULL);
		}
	}

	//supply sorting
	{
		///max_supply -- 100 %
		//supply --- x

		// x max_supply == 100 supply
		// x == (100 supply) / max_supply

		if (getSupply() == 0 || getMaxSupply() == 0)
			supply_percentage = 0;
		else
			supply_percentage = 100 * getSupply() / getMaxSupply();

		if (supply_percentage <= 0)
		{
			setSupplyStatus(NONE);
		}
		else if (supply_percentage > 0 && supply_percentage <= 30)
		{
			setSupplyStatus(LITTLE);
		}
		else if (supply_percentage > 30 && supply_percentage <= 80)
		{
			setSupplyStatus(SOME);
		}
		else if (supply_percentage > 80 && supply_percentage < 100)
		{
			setSupplyStatus(PLENTY);
		}
		else if (supply_percentage >= 100)
		{
			setSupplyStatus(FULL);
		}
	}
}

//function to build a supply unit. First parameter is how many already incomplete supplies is the limit to order another one
// if the limit is surpassed, the order is ignored to avoid waste of resources
int ProductionManager::buildSupply(int queueAmountThreshold)
{
	if (Broodwar->self()->isDefeated() || Broodwar->self()->isVictorious()) return 0;

	if (!isThereAvailableResourcesFor(race.getSupplyProvider())) return 0;

	// Iterate through all the units that we own
	for (auto &u : Broodwar->self()->getUnits())
	{
		if (!u->exists()) continue;

		Position pos = u->getPosition();
		Error lastErr = Broodwar->getLastError();
		Broodwar->registerEvent([pos, lastErr](Game*) { Broodwar->drawTextMap(pos, "%c%s", Text::White, lastErr.c_str()); },   // action
			nullptr,    // condition
			Broodwar->getLatencyFrames());  // frames to run

											// Retrieve the supply provider type in the case that we have run out of supplies
		UnitType supplyProviderType = u->getType().getRace().getSupplyProvider();
		static int lastChecked = 0;

		// If we are supply blocked and haven't tried constructing more recently
		if ((supply_status == PLENTY || supply_status == FULL || supply_status == SOME) &&
			lastChecked + 400 < Broodwar->getFrameCount() &&
			Broodwar->self()->incompleteUnitCount(supplyProviderType) <= queueAmountThreshold)
		{
			lastChecked = Broodwar->getFrameCount();

			// Retrieve a unit that is capable of constructing the supply needed
			Unit supplyBuilder = u->getClosestUnit(GetType == supplyProviderType.whatBuilds().first &&
				(IsIdle || IsGatheringMinerals) && IsOwned);
			// If a unit was found
			if (supplyBuilder && !util.isUnitDisabled(supplyBuilder))
			{
				if (supplyProviderType.isBuilding())
				{
					TilePosition targetBuildLocation = Broodwar->getBuildLocation(supplyProviderType, supplyBuilder->getTilePosition());
					if (targetBuildLocation)
					{
						// Register an event that draws the target build location
						Broodwar->registerEvent([targetBuildLocation, supplyProviderType](Game*)
						{
							Broodwar->drawBoxMap(Position(targetBuildLocation),
								Position(targetBuildLocation + supplyProviderType.tileSize()),
								Colors::Blue);
						},
							nullptr,  // condition
							supplyProviderType.buildTime() + 100);  // frames to run

																	// Order the builder to construct the supply structure
																	//Broodwar << "ENTROU" << endl;

						if (supplyBuilder->build(supplyProviderType, targetBuildLocation))
						{
							if (isThereAvailableResourcesFor(supplyProviderType))
								reserveUnitPrice(supplyProviderType);
							//Broodwar << "Reservou UM probe (100)" << endl;
						}
					}
				}
				else
				{
					// Train the supply provider (Overlord) if the provider is not a structure
					return 1;
					supplyBuilder->train(supplyProviderType);
				}
			} // closure: supplyBuilder is valid
		} // closure: insufficient supply
	}

	return 0;
}

bool ProductionManager::isReservedMineralsBiggerThan(int amount)
{
	return getReservedMinerals() > amount;
}

bool ProductionManager::isReservedGasBiggerThan(int amount)
{
	return getReservedGas() > amount;
}

bool ProductionManager::isReservedSupplyBiggerThan(int amount)
{
	return getReservedSupply() > amount;
}

bool ProductionManager::isThereAvailableMinerals()
{
	return !isReservedMineralsBiggerThan(getMinerals());
}

bool ProductionManager::isThereAvailableGas()
{
	return !isReservedGasBiggerThan(getGas());
}

bool ProductionManager::isThereAvailableSupply()
{
	return !isReservedSupplyBiggerThan(getMaxSupply() - getSupply());
}

bool ProductionManager::isThereAvailableResourcesFor(UnitType u)
{
	if (!u) return false;
	if (!isThereAvailableGas() || !isThereAvailableMinerals() || !isThereAvailableSupply()) return false;

	if ((u.mineralPrice() <= getAvailableMinerals()) && (u.gasPrice() <= getAvailableGas()) && (u.supplyRequired() <= getAvailableSupply()))
	{
		//setReserved_minerals(getReserved_minerals() + u.mineralPrice());
		//reserved_gas += u.gasPrice();
		return true;
	}
	else return false;
}

bool ProductionManager::reserveUnitPrice(UnitType u)
{
	if (!u) return false;

	if (!isThereAvailableResourcesFor(u)) return false;

	if (!isThereAvailableGas() || !isThereAvailableMinerals() || !isThereAvailableSupply()) return false;

	if ((u.mineralPrice() <= getAvailableMinerals()) && (u.gasPrice() <= getAvailableGas()) && (u.supplyRequired() <= getAvailableSupply()))
	{
		if (isThereAvailableResourcesFor(u))
		{
			setReservedMinerals(getReservedMinerals() + u.mineralPrice());
			setReservedGas(getReservedGas() + u.gasPrice());
			setReservedSupply(getReservedSupply() + u.supplyRequired());
			return true;
		}
		else return false;
	}
	else return false;
}

bool ProductionManager::dereserveUnitPrice(UnitType u)
{
	if (!u) return false;

	if ((getReservedMinerals() >= u.mineralPrice()) && (getReservedGas() >= u.gasPrice()) && (getReservedSupply() >= u.supplyRequired()))
	{
		setReservedMinerals(getReservedMinerals() - u.mineralPrice());
		setReservedGas(getReservedGas() - u.gasPrice());
		setReservedSupply(getReservedSupply() - u.supplyRequired());

		return true;
	}
	else return false;
}

void ProductionManager::update()
{
	if (Broodwar->self()->isDefeated() || Broodwar->self()->isVictorious()) return;

	updateResources();
	makeAllIdlesWork(getRefineriesAmount());
	updateResources();

	//if supplies are running low, order to build more supplies
	if (getSupplyStatus() == PLENTY || getSupplyStatus() == FULL)
	{
		UnitType supplyT = Broodwar->self()->getRace().getSupplyProvider();
		if (supplyT)
		{
			buildSupply(0);
			updateResources();
		}
	}

	if (getMineralStatus() == PLENTY)
	{
		///Broodwar << "IM RICH IM RICH IM RICH" << endl;
	}

	searchAndBuildRefinery();
	updateResources();

	//followBuildingOrder();
	//updateResources();

	if (isThereAvailableMinerals() && getSupplyStatus() != PLENTY || getSupplyStatus() != FULL)
	{
		//if(!hold_worker_production)
		makeIdleNexusBuildWorkers();
		updateResources();
	}
}

bool ProductionManager::setZealotRushQueues()
{
	//need to fix this
	//if (Broodwar->self()->getRace() != Protoss) return false;


	buildingInfo firstGate;
	firstGate.building = UnitTypes::Protoss_Gateway;
	firstGate.supply_timing = 10;
	firstGate.quantity = 1;

	buildingInfo secondGate;
	secondGate.building = UnitTypes::Protoss_Gateway;
	secondGate.supply_timing = 12;
	secondGate.quantity = 1;

	unitInfo firstZealot;
	firstZealot.unit = UnitTypes::Protoss_Zealot;
	firstZealot.supply_timing = 10;
	firstZealot.quantity = 1;

	unitInfo secondZealot;
	secondZealot.unit = UnitTypes::Protoss_Zealot;
	secondZealot.supply_timing = 12;
	secondZealot.quantity = 2;

	buildingsQueue.push_back(firstGate);
	buildingsQueue.push_back(secondGate);

	unitsQueue.push_back(firstZealot);
	unitsQueue.push_back(secondZealot);

	return true;
}

void ProductionManager::makeIdleNexusBuildWorkers()
{
	if (Broodwar->self()->isDefeated() || Broodwar->self()->isVictorious()) return;

	if (!isThereAvailableResourcesFor(race.getWorker())) return;

	for (auto &u : Broodwar->self()->getUnits())
	{
		if (!u->exists()) continue;

		if (u->getType().isResourceDepot() && u->exists()) // A resource depot is a Command Center, Nexus, or Hatchery
		{
			//if there is no mineral field close by stop making workers
			//Unit closet_mineral = u->getClosestUnit(Filter::IsMineralField);
			//if (!closet_mineral || !closet_mineral->getType().isMineralField()) return;

			if ((u->isIdle()) && !u->train(u->getType().getRace().getWorker()))
			{
				// If that fails, draw the error at the location so that you can visibly see what went wrong!
				// However, drawing the error once will only appear for a single frame
				// so create an event that keeps it on the screen for some frames
				Position pos = u->getPosition();
				Error lastErr = Broodwar->getLastError();
				Broodwar->registerEvent([pos, lastErr](Game*) { Broodwar->drawTextMap(pos, "%c%s", Text::White, lastErr.c_str()); },   // action
					nullptr,    // condition
					Broodwar->getLatencyFrames());  // frames to run
			} // closure: failed to train idle unit
			else
			{
				if (isThereAvailableResourcesFor(u->getType().getRace().getWorker()))
					reserveUnitPrice(u->getType().getRace().getWorker());
			}
			// Order the depot to construct more workers! But only when it is idle.
		}
	}// closure: unit iterator
}

void ProductionManager::makeAllIdlesWork(int refineries_amount)
{
	bool check3atGas = false;
	int amount_gathering_gas = 0;

	// need to make 3 workers at refinery work!!!

	//if we have at least one refinery check to see if at least 3 workers are gathering there
	if (refineries_amount > 0)
	{
		for (auto &u : Broodwar->self()->getUnits())
		{
			if (!u->exists()) continue;

			if (u->getType().isWorker())
			{
				if (u->isGatheringGas())
					amount_gathering_gas++;
			}
		}

		// if less than 3, order the necessary workers to achieve 3 at refinery
		if (amount_gathering_gas < 3)
		{
			for (int i = 0; i < 3 - amount_gathering_gas; i++)
			{
				if (amount_gathering_gas == 3) break;
				for (auto &u : Broodwar->self()->getUnits())
				{
					if (!u->exists()) continue;

					if (amount_gathering_gas == 3) break;

					if (u->getType().isWorker() && !u->isGatheringGas())
					{
						if (util.isUnitDisabled(u)) continue;

						if (!u->gather(u->getClosestUnit(Filter::IsRefinery)))
						{
							// If the call fails, then print the last error message
							Broodwar << Broodwar->getLastError() << std::endl;
						}
						else
						{
							break;
							amount_gathering_gas++;
						}
					}
				}
			}
		}
	}

	for (auto &u : Broodwar->self()->getUnits())
	{
		if (!u->exists()) continue;

		if (util.isUnitDisabled(u))
			continue;

		// Finally make the unit do some stuff!

		// If the unit is a worker unit
		if (u->getType().isWorker())
		{
			// if our worker is idle
			if (u->isIdle() && !u->isConstructing())
			{
				// Order workers carrying a resource to return them to the center,
				// otherwise find a mineral patch to harvest.
				if (u->isCarryingGas())
				{
					u->returnCargo();
				}
				else if (u->isCarryingMinerals())
				{
					u->returnCargo();
				}
				else if (!u->getPowerUp())  // The worker cannot harvest anything if it
				{                             // is carrying a powerup such as a flag
											  // Harvest from the nearest mineral patch or gas refinery
											  //if (!u->gather(u->getClosestUnit()->getType().isMineralField() || u->getClosestUnit()->getType().isRefinery()))
					if (!u->isGatheringGas())
					{
						if (!u->gather(u->getClosestUnit(Filter::IsMineralField)))
						{
							// If the call fails, then print the last error message
							Broodwar << Broodwar->getLastError() << std::endl;
						}
						else
						{
						}
					}
				} // closure: has no powerup
			} // closure: if idle
		}
	}
}

bool ProductionManager::makeWorkerBuild(UnitType unitToBuild, int queueAmountThreshold)
{
	if (Broodwar->self()->isDefeated() || Broodwar->self()->isVictorious()) return false;

	if (!isThereAvailableResourcesFor(unitToBuild)) return false;

	// Iterate through all the units that we own
	for (auto &u : Broodwar->self()->getUnits())
	{
		if (!u->exists()) continue;

		Position pos = u->getPosition();
		Error lastErr = Broodwar->getLastError();
		Broodwar->registerEvent([pos, lastErr](Game*) { Broodwar->drawTextMap(pos, "%c%s", Text::White, lastErr.c_str()); },   // action
			nullptr,    // condition
			Broodwar->getLatencyFrames());  // frames to run

		static int lastChecked = 0;

		// If we haven't tried constructing more recently
		if (lastChecked + 400 < Broodwar->getFrameCount() &&
			Broodwar->self()->incompleteUnitCount(unitToBuild) <= queueAmountThreshold)
		{
			lastChecked = Broodwar->getFrameCount();

			// Retrieve a unit that is capable of constructing the supply needed
			Unit builder = u->getClosestUnit(GetType == unitToBuild.whatBuilds().first &&
				(IsIdle || IsGatheringMinerals) && IsOwned);
			// If a unit was found
			if (builder && !util.isUnitDisabled(builder))
			{
				//if unit to build is a building
				if (unitToBuild.isBuilding())
				{
					TilePosition targetBuildLocation = Broodwar->getBuildLocation(unitToBuild, builder->getTilePosition());
					if (targetBuildLocation)
					{
						// Register an event that draws the target build location
						Broodwar->registerEvent([targetBuildLocation, unitToBuild](Game*)
						{
							Broodwar->drawBoxMap(Position(targetBuildLocation),
								Position(targetBuildLocation + unitToBuild.tileSize()),
								Colors::Blue);
						},
							nullptr,  // condition
							unitToBuild.buildTime() + 100);  // frames to run

															 // Order the builder to construct the supply structure
															 //Broodwar << "ENTROU" << endl;

						if (builder->build(unitToBuild, targetBuildLocation))
						{
							if (isThereAvailableResourcesFor(unitToBuild))
							{
								//maybe put inside if
								reserveUnitPrice(unitToBuild);
								return true;
							}

							//Broodwar << "Reservou UM probe (100)" << endl;
						}
					}
				}
				else
				{
					// Train the supply provider (Overlord) if the provider is not a structure

					if (builder->train(unitToBuild))
						return true;
				}
			} // closure: supplyBuilder is valid
		} // closure: insufficient supply
	}

	return false;
}

void ProductionManager::searchAndBuildRefinery()
{
	if (Broodwar->self()->isDefeated() || Broodwar->self()->isVictorious()) return;
	if (Broodwar->self()->getUnits().size() == 0) return;

	if (!isThereAvailableResourcesFor(race.getRefinery())) return;

	//first check if we don't have an refinery already
	for (auto &u : Broodwar->self()->getUnits())
	{
		if (!u->exists()) continue;

		if (u->exists() && u->getType().isRefinery())
		{
			//Broodwar << " JA TEMOSSS" << endl;

			return;
		}
	}

	// Resource_Vespene_Geyser

	for (auto &u : Broodwar->self()->getUnits())
	{
		if (!u->exists()) continue;
		static int lastChecked = 0;

		Unit geyser = u->getClosestUnit(GetType == UnitTypes::Resource_Vespene_Geyser);

		UnitType refineryType = u->getType().getRace().getRefinery();

		if (!geyser || !geyser->exists() || !refineryType || util.isUnitDisabled(geyser)) continue;
		//if (util.isUnitDisabled(u)) continue;

		// If we haven't tried constructing more recently
		if (lastChecked + 400 < Broodwar->getFrameCount() &&
			Broodwar->self()->incompleteUnitCount(refineryType) == 0)
		{
			lastChecked = Broodwar->getFrameCount();

			Unit refineryBuilder = u->getClosestUnit(GetType == refineryType.whatBuilds().first &&
				(IsIdle || IsGatheringMinerals) && IsOwned);

			if (!util.isUnitDisabled(refineryBuilder) && refineryBuilder)
			{
				if (refineryType.isBuilding())
				{
					TilePosition targetBuildLocation = Broodwar->getBuildLocation(refineryType, geyser->getTilePosition());

					if (targetBuildLocation && targetBuildLocation.isValid())
					{
						// Register an event that draws the target build location
						Broodwar->registerEvent([targetBuildLocation, refineryType](Game*)
						{
							Broodwar->drawBoxMap(Position(targetBuildLocation),
								Position(targetBuildLocation + refineryType.tileSize()),
								Colors::Blue);
						},
							nullptr,  // condition
							refineryType.buildTime() + 100);  // frames to run

															  // Order the builder to construct the supply structure
															  //Broodwar << "ENTROU" << endl;

						if (refineryBuilder->build(refineryType, targetBuildLocation))
						{
							if (isThereAvailableResourcesFor(refineryType))
								reserveUnitPrice(refineryType);
						}
					}
				}
			}
			else
			{
				// Train the refinery provider if the provider is not a structure
				//return 1;
				if (refineryBuilder->train(refineryType))
				{
					if (isThereAvailableResourcesFor(refineryType))
						reserveUnitPrice(refineryType);
				}
			}
		}
	}
}

void ProductionManager::followBuildingOrder()
{
	if (buildingsQueue.size() <= 0) return;

	nextBuidling = buildingsQueue.front();

	//if we need to build at least one building
	if (nextBuidling.quantity >= 1)
	{
		//if it is the right time and we have the resources for it
		if (nextBuidling.supply_timing >= getSupply())
		{
			Broodwar << "Need to build a: " + nextBuidling.building.getName() << std::endl;
			//isThereAvailableResourcesFor(nextBuidling.building)
			//make a worker build that building, if the quantity is more than one, it will construct even if its already being build somewhere
			makeWorkerBuild(nextBuidling.building, nextBuidling.quantity - 1);
		}
	}

	//if()
}

void ProductionManager::followUnitOrder()
{

}

//returns the amount of refineries that we own
int ProductionManager::getRefineriesAmount()
{
	int amount = 0;
	for (auto &u : Broodwar->self()->getUnits())
	{
		if (!u->exists()) continue;

		if (u->getType() == u->getType().getRace().getRefinery())
		{
			if (!util.isUnitDisabled(u))
				amount++;
		}
	}

	return amount;
}

//function to never let reserved resources be bigger than actual ones
void ProductionManager::contigencyReservedResources()
{
	if (getReservedMinerals() > getMinerals()) setReservedMinerals(getMinerals());
	if (getReservedGas() > getGas()) setReservedGas(getGas());
	if (getReservedSupply() > getAvailableSupply()) setReservedSupply(getSupply());
}

bool ProductionManager::decrementFirstBuildingInQueueOrder(Unit u)
{
	if (buildingsQueue.size() < 0) return false;

	bool decremented = false;

	for (int i = 0; i < buildingsQueue.size(); i++)
	{
		if (buildingsQueue[i].building == u->getType() && buildingsQueue[i].quantity >= 1)
		{
			decremented = true;
			buildingsQueue[i].quantity--;
		}
	}

	//update building queue to remove any building that has 0 quantity
	for (int i = 0; i < buildingsQueue.size(); i++)
	{
		if (buildingsQueue[i].quantity == 0)
		{
			buildingsQueue.erase(buildingsQueue.begin() + i);
		}
	}

	return decremented;
}

bool ProductionManager::decrementFirstUnitInQueueOrder(Unit u)
{
	if (unitsQueue.size() < 0) return false;

	bool decremented = false;

	for (int i = 0; i < unitsQueue.size(); i++)
	{
		if (unitsQueue[i].unit == u->getType() && unitsQueue[i].quantity >= 1)
		{
			decremented = true;
			unitsQueue[i].quantity--;
		}
	}

	//update building queue to remove any building that has 0 quantity
	for (int i = 0; i < unitsQueue.size(); i++)
	{
		if (unitsQueue[i].quantity == 0)
		{
			unitsQueue.erase(unitsQueue.begin() + i);
		}
	}

	return decremented;
}

bool ProductionManager::decrementFirstInQueueOrder(Unit u)
{
	if (u->getType().isBuilding())
	{
		return decrementFirstBuildingInQueueOrder(u);
	}
	else
	{
		return decrementFirstUnitInQueueOrder(u);
	}
}

bool ProductionManager::isUnitInQueueOrder(Unit u)
{
	if (u->getType().isBuilding())
	{
		for (int i = 0; i < buildingsQueue.size(); i++)
		{
			if (buildingsQueue[i].building == u->getType())
			{
				return true;
			}
		}
	}
	else
	{
		for (int i = 0; i < unitsQueue.size(); i++)
		{
			if (unitsQueue[i].unit == u->getType())
			{
				return true;
			}
		}
	}

	return false;
}

unsigned int ProductionManager::getAvailableSupply() const
{
	return getMaxSupply() - getSupply() - getReservedSupply();
}

unsigned int ProductionManager::getAvailableMinerals() const
{
	return getMinerals() - getReservedMinerals();
}

unsigned int ProductionManager::getAvailableGas() const
{
	return getGas() - getReservedGas();
}

ProductionManager::~ProductionManager()
{
}