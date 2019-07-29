#include "Simulator.h"
#include <iostream>
/**
 * Constructor for Simulator class for simulations for only species trees
 * @param p RNG seed from MrBayes code
 * @param nt number of taxa to be used in simulation
 * @param lambda  birth rate as double
 * @param mu death rate as double
 * @param rho Sampling fraction
 */
Simulator::Simulator(MbRandom *p, unsigned nt, double lambda, double mu, double rho)
{
    spTree = nullptr;
    geneTree = nullptr;
    lociTree = nullptr;
    simType = 1;
    currentSimTime = 0.0;
    rando = p;
    numTaxaToSim = nt;
    gsaStop = 100*nt;
    speciationRate = lambda;
    extinctionRate = mu;
    samplingRate = rho;
    
    
    numLoci = 0;
    numGenes = 0;
    geneBirthRate = 0.0;
    geneDeathRate = 0.0;
    transferRate = 0.0;
    propTransfer = 0.0;
    indPerPop = 0;
    popSize = 0;
    
}

/**
 * Constructor for Simulator class used in simulations of SpeciesTree and LocusTree classes only
 * @param p RNG seed from MrBayes code
 * @param ntax Number of taxa to simulate
 * @param lambda Rate of birth as double
 * @param mu Rate of death as double
 * @param rho Sampling Fraction (currently not implemented)
 * @param numLociToSim Number of loci to simulate per species tre
 * @param gbr Gene birth rate as double
 * @param gdr Gene death rate as double
 * @param lgtr Lateral gene transfer rate
 */

Simulator::Simulator(MbRandom *p, unsigned ntax, double lambda, double mu, double rho, unsigned numLociToSim, double gbr, double gdr, double lgtr)
{
    spTree = nullptr;
    geneTree = nullptr;
    lociTree = nullptr;
    simType = 2;
    currentSimTime = 0.0;
    rando = p;
    numTaxaToSim = ntax;
    gsaStop = 100*ntax;
    speciationRate = lambda;
    extinctionRate = mu;
    samplingRate = rho;
    
    numLoci = numLociToSim;
    geneBirthRate = gbr;
    geneDeathRate = gdr;
    transferRate = lgtr;
    propTransfer = 0.0;
    indPerPop = 0;
    popSize = 0;
    
    
}

/**
 * Constructor of Simulator class for full three-tree model
 * @param p RNG seed from MrBayes code
 * @param ntax Number of taxa to simulate
 * @param lambda Rate of birth as double
 * @param mu Rate of death as double
 * @param rho Sampling Fraction (currently not implemented)
 * @param numLociToSim Number of loci to simulate per species tre
 * @param gbr Gene birth rate as double
 * @param gdr Gene death rate as double
 * @param lgtr Lateral gene transfer rate
 * @param ipp Individuals per locus tree lineage to perform coalescent process on
 * @param Ne Total population size of each locus tree lineage
 * @param genTime Scaling factor for converting from coalescent units to absolute time entered as generations per unit time
 * @param ng Number of gene trees to simulate for each locus tree lineage
 * @param og Outgroup fraction, pastes a fake outgroup on tree with branch length set to some fraction of total tree depth
 * @param ts Scaling factor for tree entered as a double to convert trees to some length (e.g. 1.0)
 * @param sout Bool for turning off stdout (speeds up the program)
 */
Simulator::Simulator(MbRandom *p,
                     unsigned ntax,
                     double lambda,
                     double mu,
                     double rho,
                     unsigned numLociToSim,
                     double gbr,
                     double gdr,
                     double lgtr,
                     unsigned ipp,
                     unsigned Ne,
                     double genTime,
                     int ng,
                     double og,
                     double ts,
                     bool sout)
{
    spTree = nullptr;
    geneTree = nullptr;
    lociTree = nullptr;
    simType = 3;
    currentSimTime = 0.0;
    rando = p;
    numTaxaToSim = ntax;
    gsaStop = 100*ntax;
    speciationRate = lambda;
    extinctionRate = mu;
    samplingRate = rho;
    numLoci = numLociToSim;
    numGenes = ng;
    geneBirthRate = gbr;
    geneDeathRate = gdr;
    transferRate = lgtr;
    propTransfer = 0.0;
    indPerPop = ipp;
    popSize = Ne;
    printSOUT = sout;
    generationTime = genTime;
    outgroupFrac = og;
    geneTrees.resize(numLoci);
    treeScale = ts;
}
/**
 * Destructor for Simulator classes
 */
Simulator::~Simulator(){
    for(std::vector<SpeciesTree*>::iterator p=gsaTrees.begin(); p != gsaTrees.end(); ++p){
        delete (*p);
    }
    gsaTrees.clear();
    int i = 0;
    for(std::vector<LocusTree*>::iterator p=locusTrees.begin(); p != locusTrees.end(); ++p){
        delete (*p);
        for(std::vector<GeneTree*>::iterator q=geneTrees[i].begin(); q != geneTrees[i].end(); ++q){
            delete (*q);
        }
        geneTrees[i].clear();
        ++i;
    }
    locusTrees.clear();
    
        
}

/**
 * Function for intializing the simulation starting with creating the SpeciesTree class
 */
void Simulator::initializeSim(){
    spTree = new SpeciesTree(rando, numTaxaToSim, currentSimTime, speciationRate, extinctionRate);
}


/**
 * Generalized Sampling Algorithm for generating birth-death trees of the correct length
 *
 * @details Below is the machinery to use GSA sampling (Hartmann 2010) to simulate a species tree. Much of this code is modified from FossilGen (written by Tracy Heath)
 */
bool Simulator::gsaBDSim(){
    double timeIntv, sampTime;
    bool treeComplete = false;
    SpeciesTree st =  SpeciesTree(rando, numTaxaToSim, currentSimTime, speciationRate, extinctionRate);
    spTree = &st;
    double eventTime;
    
    while(gsaCheckStop()){
        eventTime = spTree->getTimeToNextEvent();
        currentSimTime += eventTime;
        spTree->ermEvent(currentSimTime);
        if(spTree->getNumExtant() < 1){
            treeComplete = false;
            return treeComplete;
        }
        else if(spTree->getNumExtant() == numTaxaToSim){
            timeIntv = spTree->getTimeToNextEvent();
            sampTime = rando->uniformRv(0, timeIntv) + currentSimTime;
            spTree->setPresentTime(sampTime);
            processGSASim();
        }
        
    }
    unsigned gsaRandomTreeID = rando->uniformRv(0, (unsigned) gsaTrees.size() - 1);
    // delete spTree;
    spTree = gsaTrees[gsaRandomTreeID];
    processSpTreeSim();
    spTree->setBranchLengths();
    spTree->setTreeTipNames();
    currentSimTime = spTree->getCurrentTimeFromExtant();
    if(treeScale > 0.0){
        spTree->scaleTree(treeScale, currentSimTime);
        currentSimTime = treeScale;
    }

    treeComplete = true;
    
    return treeComplete;
}

/**
 * Function for checking whether the simulation has reached the stopping point
 * @return Bool for whether to keepSimulating or not
 */

bool Simulator::gsaCheckStop(){
  
  bool keepSimulating = true;
  
  if(spTree->getNumExtant() >= gsaStop){
      keepSimulating = false;
  }
  
  return keepSimulating;
}

/**
 * Function for completing the simulation
 * @details this prunes the desired species tree from the larger simulated tree according to Hartmann et al. 2010
 */
void Simulator::processGSASim(){
    SpeciesTree *tt = new SpeciesTree(rando, numTaxaToSim + spTree->getNumExtinct());
    this->prepGSATreeForReconstruction();
    Node *simRoot = spTree->getRoot();
    tt->setRoot(simRoot);
    tt->reconstructTreeFromGSASim(simRoot);
    gsaTrees.push_back(tt);    
}

/**
 *
 */
void Simulator::processSpTreeSim(){
    spTree->setSpeciationRate(speciationRate);
    spTree->setExtinctionRate(extinctionRate);
    spTree->popNodes();

}

void Simulator::prepGSATreeForReconstruction(){
    spTree->setGSATipTreeFlags();
}

bool Simulator::simMoranSpeciesTree(){
    bool simgood = false;
    while(!simgood){
        simgood = moranSpeciesSim();
    }
    return simgood;
}

bool Simulator::moranSpeciesSim(){
    bool treeComplete;
    SpeciesTree st =  SpeciesTree(rando, numTaxaToSim, currentSimTime, speciationRate, extinctionRate);
    spTree = &st;
    spTree->initializeMoranProcess(numTaxaToSim);
    double eventTime;
    
    while(moranCheckStop()){
        eventTime = spTree->getTimeToNextEventMoran();
        currentSimTime += eventTime;
        spTree->moranEvent(currentSimTime);
        if(spTree->getNumExtant() < 1){
            treeComplete = false;
            return treeComplete;
        }
    }
    // processing will make a tree with extant and non extant tips
    //spTree->setBranchLengths();
    //spTree->setTreeTipNames();
    if(treeScale > 0.0){
        spTree->scaleTree(treeScale, currentSimTime);
        currentSimTime = treeScale;
    }

    treeComplete = true;
    
    return treeComplete;
}


bool Simulator::moranCheckStop(){
  
  bool keepSimulating = true;
  
  if((spTree->getNumExtant() * 2) <= currentSimTime){
      keepSimulating = false;
  }
  
  return keepSimulating;    ;
}

bool Simulator::simSpeciesTree(){
    bool good = false;
    while(!good){
        good = gsaBDSim();
    }
    if(outgroupFrac > 0.0)
        this->graftOutgroup(spTree, spTree->getTreeDepth());
    return good;
}

std::string Simulator::printExtSpeciesTreeNewick(){
    SpeciesTree *tt = new SpeciesTree(rando, numTaxaToSim);
    spTree->getRootFromFlags(false);
    if(outgroupFrac > 0.0){
        tt->setOutgroup(spTree->getOutgroup());
        tt->setRoot(spTree->getOutgroup()->getAnc());
    }
    else{
        tt->setRoot(spTree->getExtantRoot());
    }
    tt->setExtantRoot(tt->getRoot());
    tt->reconstructTreeFromSim(spTree->getRoot());
    std::string newickTree = tt->printExtNewickTree();
    delete tt;
    tt = nullptr;
    return newickTree;
}

std::string Simulator::printSpeciesTreeNewick(){
    return spTree->printNewickTree();
}

bool Simulator::bdsaBDSim(){
    bool treesComplete = false;
    double stopTime = spTree->getCurrentTimeFromExtant();
    double eventTime;
    bool isSpeciation;
    lociTree = new LocusTree(rando, numTaxaToSim, currentSimTime, geneBirthRate, geneDeathRate, transferRate);

    std::map<int,double> speciesBirthTimes = spTree->getBirthTimesFromNodes();
    std::map<int,double> speciesDeathTimes = spTree->getDeathTimesFromNodes();
    std::set<int> contempSpecies;
    std::pair<int, int> sibs;
    
    Node* spRoot = spTree->getRoot();
    lociTree->setStopTime(stopTime);
    currentSimTime = 0;
    if(!(contempSpecies.empty()))
        contempSpecies.clear();
    contempSpecies.insert(spRoot->getIndex());
    
    while(currentSimTime < stopTime){
        eventTime = lociTree->getTimeToNextEvent();
        currentSimTime += eventTime;
        for(std::set<int>::iterator it = contempSpecies.begin(); it != contempSpecies.end();){
            if(currentSimTime > speciesDeathTimes[(*it)]){
                isSpeciation = spTree->macroEvent((*it));
                if(isSpeciation){
                    sibs = spTree->preorderTraversalStep(*it);
                    lociTree->speciationEvent((*it), speciesDeathTimes[(*it)], sibs);
                    it = contempSpecies.erase(it);
                    it = contempSpecies.insert( it, sibs.second);
                    ++it;
                    it = contempSpecies.insert( it, sibs.first);
                }
                else{
                    if(!(spTree->getIsExtantFromIndx(*it))){
                        lociTree->extinctionEvent(*it, speciesDeathTimes[(*it)]);
                        it = contempSpecies.erase(it);
                    }
                    else{
                        ++it;
                    }
                }
            }
            else{
                ++it;
            }
            
            if(lociTree->getNumExtant() < 1){
                treesComplete = false;
                return treesComplete;
            }
            
        }
        
        if(currentSimTime >= stopTime){
            currentSimTime = stopTime;
            lociTree->setCurrentTime(stopTime);
        }
        else{
            lociTree->ermEvent(currentSimTime);
        }



    }
    lociTree->setPresentTime(currentSimTime);
    treesComplete = true;

    return treesComplete;
}

bool Simulator::simSpeciesLociTrees(){
    bool good = false;
    bool spGood = false;
    for(int i = 0; i < numLoci; i++){
        while(!good){
            while(!spGood){
                spGood = gsaBDSim();
            }
            if(outgroupFrac > 0.0)
                this->graftOutgroup(spTree, spTree->getTreeDepth());
            if(printSOUT)
                std::cout << "Simulating loci #" <<  i + 1 << std::endl;
            good = bdsaBDSim();
        }
        if(outgroupFrac > 0.0)
            this->graftOutgroup(lociTree, lociTree->getTreeDepth());

        locusTrees.push_back(lociTree);

        good = false;
    }
    return good;
}

std::string Simulator::printLocusTreeNewick(int i){
    std::string newickTree;
    std::vector<LocusTree*>::iterator it = locusTrees.begin();
    std::advance(it, i);
    newickTree = (*it)->printNewickTree();
    return newickTree;
}


std::set<double, std::greater<double> > Simulator::getEpochs(){
    std::set<double, std::greater<double> > epochs;
    std::vector<Node*> lociTreeNodes = lociTree->getNodes();
    for(std::vector<Node*>::iterator it = lociTreeNodes.begin(); it != lociTreeNodes.end(); ++it){
        if(!((*it)->getIsExtinct())){
            if((*it)->getIsTip())
                epochs.insert((*it)->getDeathTime());
            epochs.insert((*it)->getBirthTime());
        }
        else
            epochs.insert((*it)->getDeathTime());
    }
    return epochs;
}


bool Simulator::coalescentSim(){
    bool treeGood = false;
    geneTree = new GeneTree(rando, numTaxaToSim, indPerPop, popSize, generationTime);

    std::map<int,int> spToLo;

    int ancIndx;
    int epochCount = 0;

    double stopTime, stopTimeEpoch, stopTimeLoci;
    bool allCoalesced = false, deathCheck = false;
    bool is_ext;

    std::set<double, std::greater<double> > epochs = getEpochs();
    int numEpochs = (int) epochs.size();
    std::set<int> extinctFolks = lociTree->getExtLociIndx();
    std::set<int> coalescentBounds = lociTree->getCoalBounds();
    std::vector< std::vector<int> > contempLoci = lociTree->getExtantLoci(epochs);
    std::map<int, double> stopTimes = lociTree->getBirthTimesFromNodes();
    geneTree->initializeTree(contempLoci, *(epochs.begin()));
    if(outgroupFrac != 0.0)
        contempLoci[0].pop_back();
    std::set<int>::iterator extFolksIt;

    for(std::set<double, std::greater<double> >::iterator epIter = epochs.begin(); epIter != epochs.end(); ++epIter){
        currentSimTime = *epIter;
        if(epochCount != numEpochs - 1){
            epIter = std::next(epIter, 1);
            stopTimeEpoch = *epIter;
            for(int j = 0; j < contempLoci[epochCount].size(); ++j){
                extFolksIt = extinctFolks.find(contempLoci[epochCount][j]);
                is_ext = (extFolksIt != extinctFolks.end());
                if(is_ext){
                    geneTree->addExtinctSpecies(currentSimTime, contempLoci[epochCount][j]);
                    extinctFolks.erase(extFolksIt);
                }
                stopTimeLoci = stopTimes[contempLoci[epochCount][j]];
                
                if(stopTimeLoci > stopTimeEpoch){
                    stopTime = stopTimeLoci;
                    deathCheck = true;
                }
                else{
                    stopTime = stopTimeEpoch;
                    deathCheck = false;
                }

                ancIndx = lociTree->postOrderTraversalStep(contempLoci[epochCount][j]);
                allCoalesced = geneTree->censorCoalescentProcess(currentSimTime, stopTime, contempLoci[epochCount][j], ancIndx, deathCheck);
                
                
                // if all coalesced remove that loci from the matrix of loci
                if(allCoalesced){
                    int check = contempLoci[epochCount][j];
                    for(int k = epochCount + 1; k < numEpochs; k++){
                        for(int m = 0; m < contempLoci[k].size(); ++m){
                            if(contempLoci[k][m] == check){
                                contempLoci[k].erase(contempLoci[k].begin() + m);
                                break;
                            }
                        }
                    }
                }
                allCoalesced = false;
                is_ext = false;
            }
            epIter = std::prev(epIter, 1); 
        }
        else{
            // finish coalescing
            geneTree->rootCoalescentProcess(currentSimTime, outgroupFrac);
            treeGood = true;
        }
        epochCount++;
    }

    spToLo = lociTree->getLocusToSpeciesMap();
    geneTree->setIndicesBySpecies(spToLo);
    return treeGood;
}

bool Simulator::simThreeTree(){
    bool gGood = false;
    bool spGood = false;
    bool loGood = false;
    while(!spGood){
        spGood = gsaBDSim();

    }
    for(int i = 0; i < numLoci; i++){
        while(!loGood){
            if(printSOUT)
                std::cout << "Simulating loci # " <<  i + 1 << std::endl;
            loGood = bdsaBDSim();
        }
        if(outgroupFrac > 0.0){
            this->graftOutgroup(lociTree, lociTree->getTreeDepth());
        }
        for(int j = 0; j < numGenes; j++){
            while(!gGood){
                if(printSOUT)
                    std::cout << "Simulating gene # " <<  j + 1 << " of loci # " << i + 1 << std::endl;
                gGood = coalescentSim();
            }
            geneTrees[i].push_back(geneTree);

            gGood = false;
        }
        locusTrees.push_back(lociTree);
        loGood = false;
    }
    if(outgroupFrac > 0.0)
        this->graftOutgroup(spTree, spTree->getTreeDepth());

    return gGood;
}


std::string Simulator::printGeneTreeNewick(int i, int j){
    std::string newickTree;
    newickTree = geneTrees[i][j]->printNewickTree();
    return newickTree;
}

std::string Simulator::printExtantGeneTreeNewick(int i, int j){
    std::string newickTree;
    if(geneTrees[i][j]->getExtantNodes().size() > 1){
        GeneTree *tt = new GeneTree(rando, numTaxaToSim, indPerPop, popSize, generationTime);
        geneTrees[i][j]->getRootFromFlags(true);
        if(outgroupFrac > 0.0){
            tt->setOutgroup(geneTrees[i][j]->getOutgroup());
            tt->setRoot(geneTrees[i][j]->getOutgroup()->getAnc());
        }
        else
            tt->setRoot(geneTrees[i][j]->getExtantRoot());
        tt->setExtantRoot(geneTrees[i][j]->getExtantRoot());
        tt->reconstructTreeFromSim(geneTrees[i][j]->getRoot());
        newickTree = tt->printExtantNewickTree();
        delete tt;
        tt = nullptr;
    }
    else{
        newickTree = ";";
    }
    return newickTree;
}


void Simulator::graftOutgroup(Tree *tr, double trDepth){
    Node *rootN = new Node();
    Node *currRoot = tr->getRoot();
    rootN->setBirthTime(currRoot->getBirthTime());
    Node *outgroupN = new Node();
    tr->rescaleTreeByOutgroupFrac(outgroupFrac, trDepth);
    double tipTime = tr->getEndTime();
    tr->setNewRootInfo(rootN, outgroupN, currRoot, tipTime);
}

bool Simulator::simLocusGeneTrees(){
    bool loGood = false;
    bool gGood = false;
    for(int i = 0; i < numLoci; i++){
        while(!loGood){
            if(printSOUT)
                std::cout << "Simulating loci # " <<  i + 1 << std::endl;
            loGood = bdsaBDSim();
        }
        for(int j = 0; j < numGenes; j++){
            while(!gGood){
                if(printSOUT)
                    std::cout << "Simulating gene # " <<  j + 1 << " of loci # " << i + 1 << std::endl;
                gGood = coalescentSim();
            }
            geneTrees[i].push_back(geneTree);

            gGood = false;
        }
        locusTrees.push_back(lociTree);
        loGood = false;
    }
    return gGood;
}

double Simulator::calcSpeciesTreeDepth(){
    return spTree->getTreeDepth();
}

double Simulator::calcExtantSpeciesTreeDepth(){
    SpeciesTree *tt = new SpeciesTree(rando, numTaxaToSim);
    spTree->getRootFromFlags(false);
    tt->setRoot(spTree->getExtantRoot());
    tt->setExtantRoot(tt->getRoot());
    tt->reconstructTreeFromSim(tt->getExtantRoot());
    double extTreeDepth = tt->getTreeDepth();
    delete tt;
    tt = nullptr;
    return extTreeDepth;
}

double Simulator::calcLocusTreeDepth(int i){
    return locusTrees[i]->getTreeDepth();
}

int Simulator::findNumberTransfers(){
    int numTrans = 0;
    for(int i = 0; i < locusTrees.size(); i++){
        numTrans += locusTrees[i]->getNumberTransfers();
    }
    return numTrans / (int) locusTrees.size();
} 

int Simulator::findNumberDuplications(){
    int numTrans = 0;
    for(int i = 0; i < locusTrees.size(); i++){
        numTrans += locusTrees[i]->getNumberDuplications();
    }
    return numTrans / (int) locusTrees.size();
}

int Simulator::findNumberLosses(){
    int numTrans = 0;
    for(int i = 0; i < locusTrees.size(); i++){
        numTrans += locusTrees[i]->getNumberLosses();
    }
    return numTrans / (int) locusTrees.size();
}

std::vector<double> Simulator::findNumberGenerations(){
    std::vector<double> numGenerations;
    for(int i = 0; i < locusTrees.size(); i++){
        numGenerations.push_back(0.0);
        for(int j = 0; j < geneTrees.size(); j++){
            if(!(geneTrees[i].empty())){
                numGenerations[i] += geneTrees[i][j]->getTreeDepth() * popSize;
            }
        }
        numGenerations[i] /= geneTrees.size();
    }
    return numGenerations;
}

double Simulator::findTMRCAGeneTree(int i, int j){
    return geneTrees[i][j]->getTreeDepth();
}
