#include <utility>

#include "Engine.h"
/**
 * @brief Constructor for the engine class
 * @param of string giving the outfile prefix
 * @param mt integer to be used by a switch to determine which simulation function to run
 * @param sbr The species birth rate
 * @param sdr The species death rate
 * @param gbr The locus tree birth rate
 * @param gdr The locus tree death rate
 * @param lgtr The locus tree lateral gene transfer rate
 * @param ipp Individuals per population (locus tree lineage) to be sampled from multi-locus coalescent
 * @param popsize The total population size across locus tree lineages
 * @param genTime The genTime is a scaling factor that represents a number of hypothetical generations per unit time to scale coalescent trees by
 * @param sd1 The first RNG seed
 * @param sd2 The second RNG seed.
 * @param ts The scaling factor to scale the output species tree to
 * @param reps Number of replicate species trees (and subsequent tree) to simulate
 * @param ntax Number of taxa to simulate species tree to
 * @param nloci Number of locus trees to simulate per species tree
 * @param ngen Number of gene trees to simulate per gene tree
 *
 */
Engine::Engine(std::string of, int mt, double sbr, double sdr, double gbr, double gdr, double lgtr, int ipp,
               int popsize, double genTime, int sd1, int sd2, double ts, int reps, int ntax, int nloci, int ngen,
               double og, bool sout) {
    outfilename = std::move(of);
    inputSpTree = "";
    simType = mt;
    spBirthRate = sbr;
    spDeathRate = sdr;
    geneBirthRate = gbr;
    geneDeathRate = gdr;
    transferRate = lgtr;
    doScaleTree = false;
    treescale = ts;
    printOutputToScreen = sout;
    individidualsPerPop = ipp;
    populationSize = popsize;
    generationTime = genTime;
    numSpeciesTrees = reps;
    proportionToSample = 1.0;
    numTaxa = ntax;
    numLoci = nloci;
    numGenes = ngen;
    outgroupFrac = og;
    if(sd1 > 0 && sd2 > 0)
        rando.setSeed(sd1, sd2);
    else
        rando.setSeed();
    seedType gs1, gs2;
    rando.getSeed(gs1, gs2);
    std::cout << "\nSeeds = {" << gs1 << ", " << gs2 << "}" << std::endl;
}

/**
 * @brief Destructor of engine class
 *
 */

Engine::~Engine(){
    for(auto & simSpeciesTree : simSpeciesTrees){
        delete simSpeciesTree;
    }
    simSpeciesTrees.clear();
}

/**
 * @brief Function that creates a Simulator class and runs the simulation saving information in the TreeInfo class.
 *
 */
void Engine::doRunRun(){
    // double TS = 0.0;
    TreeInfo *ti = nullptr;
    for(int k = 0; k < numSpeciesTrees; k++){

        auto *treesim = new Simulator(&rando,
                                           numTaxa,
                                           spBirthRate,
                                           spDeathRate,
                                           1.0,
                                           numLoci,
                                           geneBirthRate,
                                           geneDeathRate,
                                           transferRate,
                                           individidualsPerPop,
                                           populationSize,
                                           generationTime,
                                           numGenes,
                                           outgroupFrac,
                                           treescale,
                                           printOutputToScreen);
        if(printOutputToScreen)
            std::cout << "Simulating species tree replicate # " << k + 1 << std::endl;

        switch(simType){
            case 1:
                treesim->simSpeciesTree();
                break;
            case 2:
                treesim->simSpeciesLociTrees();
                break;
            case 3:
                treesim->simThreeTree();
                break;
            case 4:
                treesim->simLocusGeneTrees();
                break;
            case 5:
                treesim->simMoranSpeciesTree();
                break;
            default:
                treesim->simSpeciesTree();
                break;
        }

        ti =  new TreeInfo(k, numLoci);
        
        ti->setWholeTreeStringInfo(treesim->printSpeciesTreeNewick());
        ti->setExtTreeStringInfo(treesim->printExtSpeciesTreeNewick());
        ti->setSpeciesTreeDepth(treesim->calcSpeciesTreeDepth());
        ti->setExtSpeciesTreeDepth(treesim->calcExtantSpeciesTreeDepth());
        ti->setNumberTransfers(treesim->findNumberTransfers());
        ti->setNumberDuplications(treesim->findNumberDuplications());
        ti->setNumberLosses(treesim->findNumberLosses());
        ti->setNumberGenerations(treesim->findAveNumberGenerations());

        for(int i = 0; i < numLoci; i++){
            ti->setLocusTreeByIndx(k, treesim->printLocusTreeNewick(i));
            if(simType == 3){
                for(int j = 0; j < numGenes; j++){
                    ti->setGeneTreeByIndx(i, j, treesim->printGeneTreeNewick(i, j));
                    ti->setExtantGeneTreeByIndx(i, j, treesim->printExtantGeneTreeNewick(i, j));
                }
            }
        }
        simSpeciesTrees.push_back(ti);
        delete treesim;
    }

    this->writeTreeFiles();
}

/**
 * @brief Writes two species tree files (one with all tips, one with only extant tips. Writes locus trees, writes gene trees. Also, writes a tree file. This function loops through the treeInfo class with information saved for each simulation and writes that information out.
 *
 *
 */
void Engine::writeTreeFiles(){

    for(auto p = simSpeciesTrees.begin(); p != simSpeciesTrees.end(); p++){
        int d = (int) std::distance(simSpeciesTrees.begin(), p);
        (*p)->writeTreeStatsFile(d, outfilename);
        (*p)->writeWholeTreeFileInfo(d, outfilename);
        (*p)->writeExtantTreeFileInfo(d, outfilename);
        for(auto i = 0; i < numLoci; i++){
            (*p)->writeLocusTreeFileInfoByIndx(d, i, outfilename);
            if(simType == 3)
                // for(int j = 0; j < numGenes; j++){
                //     (*p)->writeGeneTreeFileInfoByIndx(d, i, j, outfilename);
                // }
                (*p)->writeExtGeneTreeFileInfo(d, i, numGenes, outfilename);
        }
    }
}

/**
 * @brief Finds a specific species tree (i.e. a simulation) within a vector of the TreeInfo class and outputs that TreeInfo class.
 *
 * @param i index of the species tree to be found
 * @return TreeInfo class found at index i within a vector of TreeInfo class
 */

TreeInfo* Engine::findTreeByIndx(int i){
    TreeInfo *tf = nullptr;
    int count = 0;
    for(auto & simSpeciesTree : simSpeciesTrees){
        if(count == i){
            tf = simSpeciesTree;
            break;
        }
        else
            count++;
    }
    return tf;
}

/**
 * @brief calculates the averages root age of all the saved species trees in the simulation run. This is saved to an outfile
 * @todo connect this with the stats file output
 */
void Engine::calcAverageRootAgeSpeciesTrees(){
    std::ofstream out;
    out.open("Average_root_depths_spTree.out");
    double sumRH = 0.0;
    for(auto & simSpeciesTree : simSpeciesTrees){
        sumRH += simSpeciesTree->getSpeciesTreeDepth();
        out << simSpeciesTree->getSpeciesTreeDepth() << "\n";
    }
    out.close();

    std::cout << "\n #### Average Root Age of Species Trees is " << (double) sumRH / numSpeciesTrees << " #####" << std::endl;


}

/**
 * @brief counts the number of leaves in a Newick string. This is heaveily modified by those used in a tutorial by Paul Lewis (https://phylogeny.uconn.edu/phylogenetic-software-development-tutorial/build-a-tree-from-a-newick-description/)
 * @param spTreeStr Newick tree string input by user
 * @return unsigned int Number of leaves
 */
unsigned int Engine::countNewickLeaves(const std::string& spTreeStr){
    std::regex taxonregexpr (R"((\w*\.?[\w|\s|\.]?\w*)\:)");
    std::sregex_iterator it1(spTreeStr.begin(), spTreeStr.end(), taxonregexpr);
    std::sregex_iterator it2;
    return (unsigned) std::distance(it1, it2);
}

/**
 * @brief Strips comments from input newick string
 *
 * @param spTreeStr Species tree string input with comments
 * @return std::string Same species tree string stripped of comments
 */
std::string Engine::stripCommentsFromNewickTree(const std::string& spTreeStr){
    std::string commentlessNewick;
    std::regex commentregexpr ("\\[.*?\\]");
    commentlessNewick = std::regex_replace(spTreeStr,commentregexpr,std::string(""));
    return commentlessNewick;
}

/**
 * @brief formats the names of tips given in input Newick string files to remove spaces and other tricky characters
 * @param spTreeStr input newick tree
 *
 * @return std::string string with characters removed
 */
std::string Engine::formatTipNamesFromNewickTree(const std::string& spTreeStr){
    std::string formattedNewick;
    std::regex taxonregexpr (R"((\w*\.?[\w|\s|\.]?\w*)\:)");

    formattedNewick = std::regex_replace(spTreeStr,taxonregexpr, "'$1':");
    return formattedNewick;
}

/**
 * @brief Converts a Newick tree input into the Species Tree class.
 *
 * @param spTreeStr input Newick string
 * @return SpeciesTree* pointer to SpeciesTree class
 */
SpeciesTree* Engine::buildTreeFromNewick(const std::string& spTreeStr){
    SpeciesTree* spTree = nullptr;
    Node* currNode = nullptr;
    Node* prevNode = nullptr;
    std::string commentlessSpTreeStr;
    commentlessSpTreeStr = spTreeStr;
    commentlessSpTreeStr = stripCommentsFromNewickTree(commentlessSpTreeStr);
    numTaxa = countNewickLeaves(commentlessSpTreeStr);
    commentlessSpTreeStr = formatTipNamesFromNewickTree(commentlessSpTreeStr);
    spTree = new SpeciesTree(&rando, numTaxa);
    currNode = new Node();
    spTree->setRoot(currNode);
    currNode->setAsRoot(true);

    enum {
        Prev_Tok_LParen		= 0x01,	// previous token was a left parenthesis ('(')
        Prev_Tok_RParen		= 0x02,	// previous token was a right parenthesis (')')
        Prev_Tok_Colon		= 0x04,	// previous token was a colon (':')
        Prev_Tok_Comma		= 0x08,	// previous token was a comma (',')
        Prev_Tok_Name		= 0x10,	// previous token was a node name (e.g. '2', 'P._articulata')
        Prev_Tok_EdgeLen	= 0x20	// previous token was an edge length (e.g. '0.1', '1.7e-3')
    };
    unsigned previous = Prev_Tok_LParen;

    // Some useful flag combinations
    unsigned LParen_Valid = (Prev_Tok_LParen | Prev_Tok_Comma);
    unsigned RParen_Valid = (Prev_Tok_RParen | Prev_Tok_Name | Prev_Tok_EdgeLen);
    unsigned Comma_Valid  = (Prev_Tok_RParen | Prev_Tok_Name | Prev_Tok_EdgeLen);
    unsigned Colon_Valid  = (Prev_Tok_RParen | Prev_Tok_Name);
    unsigned Name_Valid   = (Prev_Tok_RParen | Prev_Tok_LParen | Prev_Tok_Comma);
    std::string::const_iterator newickStart = commentlessSpTreeStr.begin();
    std::string::const_iterator it = newickStart;

    for(; it != commentlessSpTreeStr.end(); ++it){
        char ch = (*it);
        if(iswspace(ch))
            continue;
        switch(ch){
            case ';': {
                std::cout << "Species tree read in successfully.\n";
                break;
            }
            case '(': {
                if(!(previous & LParen_Valid)){
                    std::cerr << "Your newick tree is not formatted properly. Exiting...\n";
                    std::cerr << "A left parenthetical is not in the right place maybe...\n";
                    exit(1);
                }
                prevNode = currNode;
                currNode =  new Node();
                currNode->setAnc(prevNode);
                prevNode->setLdes(currNode);
                previous = Prev_Tok_LParen;
                break;
            }
            case ':': {
                if(!(previous & Colon_Valid)){
                    std::cerr << "Your newick tree is not formatted properly. Exiting...\n";
                    std::cerr << "A colon appears to be in the wrong place...\n";
                    exit(1);
                }
                previous = Prev_Tok_Colon;
                break;
            }
            case ')': {
                if(!(previous & RParen_Valid)){
                    std::cerr << "Your newick tree is not formatted properly. Exiting...\n";
                    std::cerr << "A right parenthetical is not in the right place maybe...\n";
                    exit(1);
                }
                currNode = currNode->getAnc();
                previous = Prev_Tok_RParen;
                break;
            }
            case ',': {
                if(!(previous & Comma_Valid)){
                    std::cerr << "Your newick tree is not formatted properly. Exiting...\n";
                    std::cerr << "A comma is not in the right place maybe...\n";
                    exit(1);
                }
                prevNode = currNode;
                currNode = new Node();
                prevNode->setSib(currNode);
                currNode->setSib(prevNode);
                currNode->setAnc(prevNode->getAnc());
                prevNode->getAnc()->setRdes(currNode);

                previous = Prev_Tok_Comma;
                break;
            }
            case '\'': {
                std::string tipname;
                for (++it; it != commentlessSpTreeStr.end(); ++it){
                    ch = *it;
                    if (ch == '\''){
                        break;
                    }
                    else if (iswspace(ch))
                        tipname += ' ';
                    else
                        tipname += ch;
                }
                if(previous == Prev_Tok_RParen){
                    //prevNode->setIsTip(true);
                //    prevNode->setName(tipname);
                }
                else if(previous == Prev_Tok_Comma){
                    currNode->setIsTip(true);
                    currNode->setName(tipname);
                }
                else if(previous == Prev_Tok_LParen){
                    currNode->setName(tipname);
                    currNode->setIsTip(true);
                }

                previous = Prev_Tok_Name;
                break;
            }
            default: {
                // branch length
                if(previous == Prev_Tok_Colon){
                    std::string::const_iterator jit = it;
                    for (; it != commentlessSpTreeStr.end(); ++it){
                        ch = *it;
                        if (ch == ',' || ch == ')' || iswspace(ch)){
                            --it;
                            break;
                        }
                        bool valid = (ch =='e' || ch == 'E' || ch =='.' || ch == '-' || ch == '+' || isdigit(ch));
                        if (!valid){
                            std::cerr << "Invalid branch length character in tree description\n";
                            exit(1);
                        }
                        std::string edge_length_str = std::string(jit,it+1);
                        currNode->setBranchLength(atof(edge_length_str.c_str()));
                        if (currNode->getBranchLength() < 1.e-10)
                            currNode->setBranchLength(1.e-10);
                        previous = Prev_Tok_EdgeLen;
                    }
                }
                else{
                    // Get the node name
                    std::string tipname;
                    for (; it != commentlessSpTreeStr.end(); ++it){
                        ch = *it;
                        if (ch == '('){
                            std::cerr << "Didn't expect a left parenthesis here! Check your newick tree...\n";
                            std::cerr << "Exiting... :(\n";
                            exit(1);
                        }
                        if (iswspace(ch) || ch == ':' || ch == ',' || ch == ')'){
                            --it;
                            break;
                        }
                        tipname += ch;
                    }

                    // Expect node name only after a left paren (child's name), a comma (sib's name) or a right paren (parent's name)
                    if (!(previous & Name_Valid)){
                        std::cerr << "Unexpected placement of name of tip. Exiting...\n";
                        exit(1);
                    }
                    currNode->setIsTip(true);
                    currNode->setName(tipname);
                    previous = Prev_Tok_Name;
                }
                if(it == commentlessSpTreeStr.end())
                    break;
            }

        }

    }
    spTree->popNodes();
    spTree->setTreeInfo();
    return spTree;
}
/**
 * @brief Called from doRunRun if Species tree is read in, simulates a locus tree, then gene trees.
 *
 *
 */
void Engine::doRunSpTreeSet(){

    std::cout << "Setting species tree to this newick tree: " << inputSpTree << std::endl;

    TreeInfo *ti = nullptr;
    auto *treesim = new Simulator(&rando,
                                        numTaxa,
                                        spBirthRate,
                                        spDeathRate,
                                        1.0,
                                        numLoci,
                                        geneBirthRate,
                                        geneDeathRate,
                                        transferRate,
                                        individidualsPerPop,
                                        populationSize,
                                        generationTime,
                                        numGenes,
                                        outgroupFrac,
                                        treescale,
                                        printOutputToScreen);


    treesim->setSpeciesTree(this->buildTreeFromNewick(inputSpTree));
    treesim->simLocusGeneTrees();


    ti =  new TreeInfo(0, numLoci);
    ti->setWholeTreeStringInfo(treesim->printSpeciesTreeNewick());
    ti->setSpeciesTreeDepth(treesim->calcSpeciesTreeDepth());
    ti->setExtSpeciesTreeDepth(treesim->calcExtantSpeciesTreeDepth());
    ti->setNumberTransfers(treesim->findNumberTransfers());
    for(int i = 0; i < numLoci; i++){
        ti->setLocusTreeByIndx(i, treesim->printLocusTreeNewick(i));
        if(simType == 3){
            for(int j = 0; j < numGenes; j++){
                ti->setGeneTreeByIndx(i, j, treesim->printGeneTreeNewick(i, j));
                ti->setExtantGeneTreeByIndx(i, j, treesim->printExtantGeneTreeNewick(i, j));

            }
        }
    }
    simSpeciesTrees.push_back(ti);
    delete treesim;

    this->writeTreeFiles();

}

void Engine::setInputSpeciesTree(const std::string &stNewick) { inputSpTree = stNewick; }


/**
 * @brief Constructor of TreeInfo class used to store simulated information.
 *
 * @param idx index of where the TreeInfo is stored in a vector
 * @param nl number of gene trees to simulate
 */
TreeInfo::TreeInfo(int idx, int nl){
    geneTrees.resize(nl);
    extGeneTrees.resize(nl);
    spTreeLength = 0.0;
    extSpTreeLength = 0.0;
    spTreeDepth = 0.0;
    extSpTreeDepth = 0.0;
    spTreeNess = 0.0;
    spAveTipLen = 0.0;
    loTreeLength = 0.0;
    loTreeDepth = 0.0;
    loTreeNess = 0.0;
    loAveTipLen = 0.0;
    aveTMRCAGeneTree = 0.0;
    numTransfers = 0;
    numDuplications = 0;
    numLosses = 0;
}

/**
 * @brief Destructor of TreeInfo class.
 *
 */
TreeInfo::~TreeInfo(){
    gsaTrees.clear();
    locusTrees.clear();
    geneTrees.clear();
    extGeneTrees.clear();
    speciesTree.clear();
}

/**
 * @brief writes a tree stats file out
 *
 * @param spIndx index of the simulation to write the statistics out of
 * @param ofp string of the outfile prefix
 */
void TreeInfo::writeTreeStatsFile(int spIndx, std::string ofp){
    std::string path;
    std::string fn = std::move(ofp);
    std::stringstream tn;
    tn << spIndx;
    fn += "_" + tn.str() + ".sp.tre.stats.txt";
    path += fn;
    std::ofstream out(path);
    out << "Species Tree Statistics\n";

    tn.clear();
    tn.str(std::string());
    tn << getSpeciesTreeDepth();
    out << "Tree depth\t" << tn.str() << std::endl;

    tn.clear();
    tn.str(std::string());

    tn << getExtSpeciesTreeDepth();
    out << "Extant Tree depth\t" << tn.str() << std::endl;

    tn.clear();
    tn.str(std::string());
    // tn << getSpeciesAveTipLen();
    // out << "Average branch length: " << tn.str() << std::endl;
    tn << getNumberTransfers();
    out << "Avaerage Transfers\t" << tn.str() << std::endl;
    

    tn.clear();
    tn.str(std::string());
    tn << getNumberDuplications();
    out << "Average Duplications\t" << tn.str() << std::endl;
    
    tn.clear();
    tn.str(std::string());
    tn << getNumberLosses();
    out << "Average Losses\t" << tn.str() << std::endl;
    
    tn.clear();
    tn.str(std::string());
    out << "Average generations of gene trees per locus tree" << std::endl;
    for(int i = 0; i < locusTrees.size(); i++){
        tn.clear();
        tn.str(std::string());
        tn << getNumberGenerationsByLindx(i);
        out << "Locus Tree " << std::to_string(i) << "\t" << tn.str() << std::endl;
    }
}

/**
 * @brief writes out tree with all lineages as a nexus file
 *
 * @param spIndx species tree index of species tree to be written out
 * @param ofp string of the outfile prefix
 *
 */

void TreeInfo::writeWholeTreeFileInfo(int spIndx, std::string ofp){
    std::string path;

    std::string fn = std::move(ofp);
    std::stringstream tn;

    tn << spIndx;
    //path += tn.str();
    // path +=  "/";


    fn += "_" + tn.str() + ".sp.full.tre";
    path += fn;
    std::ofstream out(path);
    out << "#NEXUS\nbegin trees;\n    tree wholeT_" << spIndx << " = ";
    out << getWholeSpeciesTree() << "\n";
    out << "end;";

}

/**
 * @brief writes out tree with only extant lineages as a nexus file
 *
 * @param spIndx species tree index of species tree to be written out
 * @param ofp string of the outfile prefix
 *
 */

void TreeInfo::writeExtantTreeFileInfo(int spIndx, std::string ofp){
   std::string path;

    std::string fn = std::move(ofp);
    std::stringstream tn;

    tn << spIndx;
    //path += tn.str();
    // path +=  "/";


    fn += "_" + tn.str() + ".sp.tre";
    path += fn;
    std::ofstream out(path);
    out << "#NEXUS\nbegin trees;\n    tree extT_" << spIndx << " = ";
    out << getExtantSpeciesTree() << "\n";
    out << "end;";
}

/**
 * @brief writes out locus tree with all lineages as a nexus file
 *
 * @param spIndx species tree index of species tree to be written out
 * @param indx index of locus tree within species tree of index spIndx
 * @param ofp string of the outfile prefix
 *
 */
void TreeInfo::writeLocusTreeFileInfoByIndx(int spIndx, int indx, std::string ofp){
    std::string path;

    std::string fn = std::move(ofp);
    std::stringstream tn;

    tn << spIndx;
    // path += tn.str();
    // path += "/";


    fn += "_" + tn.str();

    tn.clear();
    tn.str(std::string());

    tn << indx;

    fn += "_" + tn.str() + ".loc.tre";
    path += fn;

    std::ofstream out(path);
    out << "#NEXUS\nbegin trees;\n    tree locT_" << indx << " = ";
    out << getLocusTreeByIndx(indx) << "\n";
    out << "end;";
}

/**
 * @brief writes out a gene tree of a specific locus tree within a specific species tree replicate
 *
 * @param spIndx replicate index
 * @param Lindx index of locus tree within species tree of index spIndx
 * @param indx index of gene tree within locus tree of index Lindx
 * @param ofp string of outfile prefix name
 */
void TreeInfo::writeGeneTreeFileInfoByIndx(int spIndx, int Lindx, int indx, std::string ofp){
    std::string path;

    std::string fn = std::move(ofp);
    std::stringstream tn;

    tn << spIndx;
    //path += tn.str();
   // path += "/";
//TODO: not to deleted work to be done on this guy

    fn += "_" + tn.str();

    tn.clear();
    tn.str(std::string());

    tn << Lindx;

    fn += "_" + tn.str();
    tn.clear();
    tn.str(std::string());

    tn << indx;
    fn += "_" + tn.str() + ".gen.tre";
    path += fn;

    std::ofstream out(path);
    out << "#NEXUS\nbegin trees;\n    tree geneT_" << indx << " = ";
    out << getGeneTreeByIndx(Lindx, indx) << "\n";
    out << "tree extGeneT_" << indx << " = ";
    out << getExtGeneTreeByIndx(Lindx, indx) << "\n";
    out << "end;";

}

/**
 * @brief writes out all gene trees with only extant taxa of a specific locus tree within a specific species tree replicate into a nexus file
 *
 * @param spIndx replicate index
 * @param Lindx index of locus tree within species tree of index spIndx
 * @param numGenes size of gene trees to be output into a nexus file
 * @param ofp string of outfile prefix name
 */
void TreeInfo::writeExtGeneTreeFileInfo(int spIndx, int Lindx, int numGenes, std::string ofp){
    std::string path;

    std::string fn = std::move(ofp);
    std::stringstream tn;

    tn << spIndx;
    //path += tn.str();
    // path += "/";
    fn += "genetrees_";

    fn += tn.str();

    tn.clear();
    tn.str(std::string());

    tn << Lindx;

    fn += "_" + tn.str() + ".tre";
    tn.clear();
    tn.str(std::string());

    // tn << i;
    // fn += "_" + tn.str() + ".gen.tre";
    path += fn;

    std::ofstream out(path);
    // out << "#NEXUS\nbegin trees;\n";
    // for(int i = 0; i < numGenes; i++){
    //         out << "tree geneT_" << indx << " = ";
    //         out << getGeneTreeByIndx(Lindx, indx) << "\n";
    // }
    // out << "end;";


    out << "#NEXUS\nbegin trees;\n";
    for(int i = 0; i < numGenes; i++){
        out << "tree extGeneT_" << i << " = ";
        out << getExtGeneTreeByIndx(Lindx, i) << "\n";
    }
    out << "end;";

}


/**
 * @brief writes out all gene trees with all taxa of a specific locus tree within a specific species tree replicate into a nexus file
 *
 * @param spIndx replicate index
 * @param Lindx index of locus tree within species tree of index spIndx
 * @param numGenes size of gene trees to be output into a nexus file
 * @param ofp string of outfile prefix name
 */
void TreeInfo::writeGeneTreeFileInfo(int spIndx, int Lindx, int numGenes, std::string ofp){
    std::string path;

    std::string fn = std::move(ofp);
    std::stringstream tn;

    tn << spIndx;
    //path += tn.str();
    // path += "/";
    fn += "genetrees_";

    fn += tn.str();

    tn.clear();
    tn.str(std::string());

    tn << Lindx;

    fn += "_" + tn.str() + ".tre";
    tn.clear();
    tn.str(std::string());

    // tn << i;
    // fn += "_" + tn.str() + ".gen.tre";
    path += fn;

    std::ofstream out(path);
    // out << "#NEXUS\nbegin trees;\n";
    // for(int i = 0; i < numGenes; i++){
    //         out << "tree geneT_" << indx << " = ";
    //         out << getGeneTreeByIndx(Lindx, indx) << "\n";
    // }
    // out << "end;";


    out << "#NEXUS\nbegin trees;\n";
    for(int i = 0; i < numGenes; i++){
        out << "tree geneT_" << i << " = ";
        out << getExtGeneTreeByIndx(Lindx, i) << "\n";
    }
    out << "end;";

}
