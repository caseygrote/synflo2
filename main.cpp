
#include <sifteo.h>
#include <sifteo/menu.h>
#include "assets.gen.h"
#include <node.h>
#include <sifteo/usb.h>
#include <sifteo/math.h>


using namespace Sifteo;


static const unsigned gNumCubes = 2;
static struct MenuItem Mredgene[] = { { &redgene, &LabelEmpty }, { NULL, NULL } };
static struct MenuItem Mredgeneconnect[] = { { &redgeneconnect, &LabelEmpty }, { NULL, NULL } };

static struct MenuItem Mplasmid[] = { { &plasmid, &LabelEmpty }, { NULL, NULL } };
static struct MenuItem Mplasmidfillred1[] = { { &plasmidfillred1, &LabelEmpty }, { NULL, NULL } };
static struct MenuItem Mplasmidfillred2[] = { { &plasmidfillred2, &LabelEmpty }, { NULL, NULL } };
static struct MenuItem Mplasmidfillred3[] = { { &plasmidfillred3, &LabelEmpty }, { NULL, NULL } };
static struct MenuItem Mplasmidfillred4[] = { { &plasmidfillred4, &LabelEmpty }, { NULL, NULL } };

static struct MenuAssets cubeAssets = { &BgTile, &Footer, &LabelEmpty, { &Tip0, &Tip1, &Tip2, NULL } };


static Menu menus[gNumCubes];
static Menu menuStore[gNumCubes];
static VideoBuffer v[gNumCubes];
static struct MenuEvent events[gNumCubes];
static uint8_t cubetouched;
static bool flipped[gNumCubes] = { false };
static bool locked[gNumCubes] = { false };

CubeID stack[gNumCubes + 1] = {-1};
int stackPointer = 0;

//NODES: 
static const unsigned numNodes = 32;
static Node nodeItems[numNodes+1]; //all node items 
static Node currentNode[gNumCubes + 1]; //node items assoc. with cubes 

static unsigned gNumCubesConnected = CubeSet::connected().count();

static AssetSlot MainSlot = AssetSlot::allocate()
.bootstrap(BetterflowAssets);

static TiltShakeRecognizer motion[gNumCubes]; //for keeping track of each cube's motion @ev
static unsigned currentScreen[gNumCubes]; //for keeping track of each cube's current screen @ev
static unsigned currentScreenStore[gNumCubes] = { 0 };

static unsigned currPart[gNumCubes][8] = { { 1 } };

UsbPipe <3, 4> usbPipe;

static unsigned shakecount=0;

static Metadata M = Metadata()
.title("helo")
.package("com.sifteo.sdk.menuhello", "1.0.0")
.icon(Icon)
.cubeRange(gNumCubes);


/*EVENTSENSOR CLASS
based on codebase from LSU CCT*/
class EventSensor{
public:
	void install(){
		Events::cubeAccelChange.set(&EventSensor::onAccelChange, this);
	}

private:
	void onAccelChange(unsigned id){

		unsigned changeFlags = motion[id].update(); 
		if (changeFlags){
			if (motion[id].shake && !locked[id]){
				LOG("SHAKING\n");
				v[id].attach(id); //shaking gets rid of selected part (i.e. you can scroll menu again) @ev
				//////////////////////////////////////////////////////////////////////////////////trashlord was here
					if(shakecount == 0){
						shakecount++;
						menus[1].init(v[id], &cubeAssets, Mplasmidfillred1);
						//menus[0].init(v[id], &cubeAssets, Mredgeneconnect);
					}
					else if(shakecount == 1){
						shakecount++;
						menus[1].init(v[id], &cubeAssets, Mplasmidfillred2);
						//menus[0].init(v[id], &cubeAssets, Mredgeneconnect);
					}
					else if(shakecount == 2){
						shakecount++;
						menus[1].init(v[id], &cubeAssets, Mplasmidfillred3);
						//menus[0].init(v[id], &cubeAssets, Mredgeneconnect);
					}
					else if(shakecount == 3){
						shakecount++;
						menus[1].init(v[id], &cubeAssets, Mplasmidfillred4);
						//menus[0].init(v[id], &cubeAssets, Mredgeneconnect);
					}
					else if(shakecount == 4){
						shakecount=0;
						menus[1].init(v[id], &cubeAssets, Mplasmid);
						//menus[0].init(v[id], &cubeAssets, Mredgene);
					}
				

				//currentNode[id] = nodeItems[0]; //assigns top level node @ev
			}
			else if (motion[id].Tilt_ZChange){
				if (motion[id].tilt.z == -1 && motion[id].tilt.x == 0 && motion[id].tilt.y == 0 && !flipped[id] && locked[id]){
					flipped[id] = true;
					LOG("flipped\n");
					//handleStack(id, 1);
				}
				else if (motion[id].tilt.z == 1 && motion[id].tilt.x == 0 && motion[id].tilt.y == 0 && flipped[id]){
					flipped[id] = false;
					LOG("flipped back\n");
					//handleStack(id, 0);
				}
			}
		}
	}

	//static void handleStack(CubeID flipped, int binary){
	//	Node NoMore = currentNode[gNumCubes];
	//	if (binary == 1){
	//		LOG("stacking\n");
	//		stack[stackPointer] = flipped;
	//		stackPointer++;
	//	}
	//	else {
	//		if (flipped == stack[stackPointer-1]){
	//			if (stackPointer == 1){
	//				LOG("bottom-most cube\n");
	//				stackPointer--;
	//				static struct MenuAssets newAssets = { &NewBgTile, &newFooter, &newLabelEmpty, { NULL } };
	//				menuStore[flipped].init(v[flipped], &cubeAssets, menus[flipped].items); //storing old one 
	//				menus[flipped].init(v[flipped], NoMore.getAssets(), construct);
	//				stack[0] = -1; //essentially "clearing" stack
	//			}
	//			else {
	//				LOG("other cube\n");
	//				stackPointer--;
	//				menuStore[flipped].init(v[flipped], &cubeAssets, menus[flipped].items);
	//				menus[flipped].init(v[flipped], NoMore.getAssets(), NoMore.getMenu());
	//				currentNode[flipped] = NoMore;
	//			}
	//		}
	//		else {
	//			stackPointer = 0;
	//			//return everyone to their originals?
	//			
	//		}

	//	}
	//	locked[flipped] = false;
	//	LOG("stackPointer:%d\n", stackPointer);
	//}
};




/* BEGIN METHOD
attaches video buffers to all connected cubes*/
static void begin(){	
	for (CubeID cube : CubeSet::connected())
	{
		LOG("cube  %d\n", (int)cube);
		//attaching video buffers 
		auto &vid = v[cube];
		vid.initMode(BG0);
		vid.attach(cube);
		//vid.bg0.erase(StripeTile);
		//initializing and attaching motion recognizers 
		motion[cube].attach(cube);
	}
}

/*LEVEL HELPER METHOD
uses tree structure instead of character array*/
void level(unsigned id, PCubeID addedCube){
	/*LOG("In level method\n");
	unsigned screen = currentScreen[id];
	Node tr = currentNode[id];
	if (tr.getChildren() == NULL) {
		LOG("NO CHILDREN\n");
		Node noMore = currentNode[gNumCubes];
		menus[addedCube].init(v[addedCube], noMore.getAssets(), noMore.getMenu());
		currentNode[addedCube] = noMore;
	}
	else {
		Node newtr = tr.getChildren()[screen+1];
		LOG("size is %d", sizeof(tr.getChildren()));
		if (currentNode[addedCube].getMenu() != newtr.getMenu()){
			menus[addedCube].init(v[addedCube], newtr.getAssets(), newtr.getMenu());
			LOG("Successfully initialized\n");
			currentNode[addedCube] = newtr;
		}
	}*/
}

/* PLUS CUBE HELPER METHOD
uses tree structure instead of character array; 
fires when cube is neighboured*/
void plusCube(unsigned id, struct MenuEvent e){
	/*LOG("In the plusCube method\n");
	if (e.neighbor.masterSide == BOTTOM && e.neighbor.neighborSide == TOP){
		CubeID(id).detachVideoBuffer();
		PCubeID addedCube = e.neighbor.neighbor;
		level(id, addedCube);
		for (int i = 0; i < 8; i++){
			currPart[addedCube][i] = currPart[id][i];
		}
		currPart[addedCube][currentNode[addedCube].getLevel() * 2] = 0;
		currPart[addedCube][currentNode[addedCube].getLevel() * 2 + 1] = 0;
	}*/
}

/* WRITE METHOD
for sending messages through usb pipe
based on usb sample code provided by sifteo*/
void write(CubeID id){
	//usbPipe.attach();

	//if (Usb::isConnected() && usbPipe.writeAvailable()) {

	//	//create message: 
	//	int message = 0;
	//	for (int i = 0; i < 8; i++){
	//		double ten = pow(10.0, 7.0-i);
	//		LOG("multiply by: %f ", ten);
	//		message = message + (ten * currPart[id][i]);
	//		LOG("index: %d, number: %d\n", i, currPart[id][i]);
	//	}
	//	UsbPacket &packet = usbPipe.sendQueue.reserve();
	//	packet.setType(0);
	//	packet.bytes()[0] = (uint8_t)69696969;
	//	packet.bytes()[1] = (uint8_t)id;
	//	packet.bytes()[2] = (uint8_t)message;

	//	LOG("Sending: %d bytes, type=%02x, data=%19h\n",
	//		packet.size(), packet.type(), packet.bytes());

	//	LOG("Should look like: %d, %d\n", (uint8_t)id, (uint8_t)message);
	//	LOG("Non-uint8_t: %d\n", message);

	//	usbPipe.sendQueue.commit();
	//}
	//usbPipe.detach();
}

void unWrite(CubeID id){
	/*usbPipe.attach();
	if (Usb::isConnected() && usbPipe.writeAvailable()){
		UsbPacket &packet = usbPipe.sendQueue.reserve();
		packet.setType(0);
		packet.bytes()[0] = (uint8_t)69696969;
		packet.bytes()[1] = (uint8_t)id;
		packet.bytes()[2] = (uint8_t)22222222;
		usbPipe.sendQueue.commit();
	}

	usbPipe.detach();*/
}

/*LOCK CUBE HELPER METHOD
also sends part info to surface*/
void lockCube(unsigned id, struct MenuEvent e){
	if (!flipped[id]){
		if (locked[id]){
			////unlock
			//LOG("current screen is %d\n", currentScreen[id]);
			////menus[id].init(v[id], &cubeAssets, menuStore[id].items);
			//menus[id].anchor(currentScreenStore[id]);
			////Sifteo::AudioChannel(0).play(WaterDrop);
			locked[id] = false;
			//unWrite(id);
		}
		else {
			//currentScreenStore[id] = currentScreen[id];
			//static struct MenuAssets newAssets = { &NewBgTile, &newFooter, &newLabelEmpty, { NULL } };
			//struct MenuItem newItems[] = { menus[id].items[e.item], { NULL, NULL } };
			//menuStore[id].init(v[id], &cubeAssets, menus[id].items);
			//menus[id].init(v[id], &cubeAssets, newItems);
			//sifteo::AudioChannel(0).play(WaterDrop);
			locked[id] = true;
			//write(id);
		}
	}
}


/* DO IT METHOD
handles event cases, takes in Menu and MenuEvent parameters*/
void __declspec(noinline) doit(Menu &m, struct MenuEvent &e, unsigned id)
{
	if (m.pollEvent(&e)){

		switch (e.type){

			{ case MENU_ITEM_PRESS:
				lockCube(id, e);
				break;
			}

			{
			case MENU_EXIT:
				ASSERT(false);
				break;
		}

			{case MENU_NEIGHBOR_ADD:
				LOG("found cube %d on side %d of menu (neighbor's %d side)\n",
					e.neighbor.neighbor, e.neighbor.masterSide, e.neighbor.neighborSide);
				/*if (!locked[id] && !locked[e.neighbor.neighbor]){
					plusCube(id, e);
				}*/
				break;

			}

			{case MENU_NEIGHBOR_REMOVE:
				LOG("lost cube %d on side %d of menu (neighbor's %d side)\n",
					e.neighbor.neighbor, e.neighbor.masterSide, e.neighbor.neighborSide);
				/*if (e.neighbor.masterSide == BOTTOM && e.neighbor.neighborSide == TOP){
					v[id].attach(id);
				}*/
				break;
			}

			{case MENU_ITEM_ARRIVE:
				//LOG("arriving at menu item %d\n", e.item);
				/*currentScreen[id] = e.item;
				unsigned level = currentNode[id].getLevel();
				if (e.item == 0){
					currPart[id][level * 2] = 0;
					currPart[id][level * 2 + 1] = 0;
					LOG("writing array index %d\n", level * 2);
				}
				else if (e.item == 1){
					currPart[id][level * 2] = 0;
					currPart[id][level * 2 + 1] = 1;
					LOG("writing array index %d\n", level * 2);
				}
				else if (e.item == 2){
					currPart[id][level * 2] = 1;
					currPart[id][level * 2 + 1] = 0;
					LOG("writing array index %d\n", level * 2);
				}
				else if (e.item == 3){
					currPart[id][level * 2] = 1;
					currPart[id][level * 2 + 1] = 1;
					LOG("writing array index %d\n", level * 2);
				}*/
				break;
			}

			{case MENU_ITEM_DEPART:
				break;
			}

			{case MENU_PREPAINT:
				// do your implementation-specific drawing here
				// NOTE: this event should never have its default handler skipped.
				break;
			}

			{case MENU_UNEVENTFUL:
				ASSERT(false);
				break;
			}

		}
		m.performDefault();
	}
	else {
		//ASSERT(e.type == MENU_EXIT);
		m.performDefault();
	}
}


/* ASSIGN TREES METHOD
for assigning menus to tree objects*/
void assign_Nodes(Node* nodeArray){
	////LEVEL 0
	//nodeArray[0] = Node(plasmid, &cubeAssets, 0);
	//nodeArray[31] = Node(noItems, &cubeAssets, 0);

	////LEVEL 1
	//nodeArray[1] = Node(promItems, &cubeAssets, 1);
	//nodeArray[2] = Node(rbsItems, &cubeAssets, 1);
	//nodeArray[3] = Node(cdsItems, &cubeAssets, 1);
	//nodeArray[4] = Node(termItems, &cubeAssets, 1);

	////LEVEL 2
	//nodeArray[5] = Node(promEcoliYeast, &cubeAssets, 2);
	//nodeArray[6] = Node(promEcoliYeast, &cubeAssets, 2);

	//nodeArray[7] = Node(rbsConst, &cubeAssets, 2);
	//nodeArray[8] = Node(rbsRibo, &cubeAssets, 2);
	//nodeArray[9] = Node(rbsYeast, &cubeAssets, 2);

	//nodeArray[10] = Node(cdsReport, &cubeAssets, 2);
	//nodeArray[11] = Node(cdsSelect, &cubeAssets, 2);
	//nodeArray[12] = Node(cdsTrans, &cubeAssets, 2);

	//nodeArray[13] = Node(termEcoli, &cubeAssets, 2);
	//nodeArray[14] = Node(termYeast, &cubeAssets, 2);
	//nodeArray[15] = Node(termEuk, &cubeAssets, 2);

	////LEVEL 3
	//nodeArray[16] = Node(promEcPos, &cubeAssets, 3);
	//nodeArray[17] = Node(promEcConst, &cubeAssets, 3);
	//nodeArray[18] = Node(promEcNeg, &cubeAssets, 3);

	//nodeArray[19] = Node(promYePos, &cubeAssets, 3);
	//nodeArray[20] = Node(promYeConst, &cubeAssets, 3);
	//nodeArray[21] = Node(promYeNeg, &cubeAssets, 3);
	//nodeArray[22] = Node(promYeMulti, &cubeAssets, 3);

	//nodeArray[23] = Node(cdsRepChromo, &cubeAssets, 3);
	//nodeArray[24] = Node(cdsRepFluor, &cubeAssets, 3);
	//nodeArray[25] = Node(cdsTransAct, &cubeAssets, 3);
	//nodeArray[26] = Node(cdsTransRep, &cubeAssets, 3);
	//nodeArray[27] = Node(cdsTransMult, &cubeAssets, 3);

	//nodeArray[28] = Node(termEcFor, &cubeAssets, 3);
	//nodeArray[29] = Node(termEcRev, &cubeAssets, 3);
	//nodeArray[30] = Node(termEcBi, &cubeAssets, 3);

	////setting level 3 children
	//Node promEcoliArray[5];
	//for (int i = 1; i < 4; i++){
	//	promEcoliArray[i] = nodeArray[i + 15];
	//}
	//promEcoliArray[4] = nodeArray[31];
	//nodeArray[5].setChildren(promEcoliArray);

	//Node promYeastArray[5];
	//promYeastArray[1] = nodeArray[19];
	//promYeastArray[2] = nodeArray[20];
	//promYeastArray[3] = nodeArray[21];
	//promYeastArray[4] = nodeArray[22];
	//nodeArray[6].setChildren(promYeastArray);

	//Node cdsRepArray[3];
	//cdsRepArray[1] = nodeArray[23];
	//cdsRepArray[2] = nodeArray[24];
	//nodeArray[10].setChildren(cdsRepArray);

	//Node cdsTransArray[4];
	//cdsTransArray[1] = nodeArray[25];
	//cdsTransArray[2] = nodeArray[26];
	//cdsTransArray[3] = nodeArray[27];
	//nodeArray[12].setChildren(cdsTransArray);

	//Node termEcArray[4];
	//termEcArray[1] = nodeArray[28];
	//termEcArray[2] = nodeArray[29];
	//termEcArray[3] = nodeArray[30];
	//nodeArray[13].setChildren(termEcArray);

	////creating and setting promoter children 
	//Node promArray[3];
	//promArray[1] = nodeArray[5];
	//promArray[2] = nodeArray[6];
	//nodeArray[1].setChildren(promArray);

	////creating and setting rbs children
	//Node rbsArray[4];
	//for (int i = 1; i < 4; i++){
	//	rbsArray[i] = nodeArray[i+6];
	//}
	//nodeArray[2].setChildren(rbsArray);

	////creating and setting cds children
	//Node cdsArray[4];
	//cdsArray[1] = nodeArray[10];
	//cdsArray[2] = nodeArray[11];
	//cdsArray[3] = nodeArray[12];
	//nodeArray[3].setChildren(cdsArray);

	////creating and setting term children
	//Node termArray[4];
	//for (int i = 1; i < 4; i++){
	//	termArray[i] = nodeArray[i + 12];
	//}
	//nodeArray[4].setChildren(termArray);

	////creating & setting top level children
	//Node topArray[5];
	//for (int i = 1; i < 5; i++){
	//	topArray[i] = nodeArray[i];
	//}
	//nodeArray[0].setChildren(topArray);
}


/* MAIN METHOD
contains begin(), initializes the MenuEvent array, 
initializes menus, & contains doit while loop*/
void main(){

	assign_Nodes(nodeItems);
	currentNode[gNumCubes] = nodeItems[numNodes - 1];

	begin();

	static EventSensor event;
	event.install();

	//usbPipe.attach();

	struct MenuEvent e;

	menus[0].init(v[0], &cubeAssets, Mplasmid);
	menus[0].anchor(0);
	menus[1].init(v[1], &cubeAssets, Mredgene);
	menus[1].anchor(0);
	

	while (1){
		
		for (int i = 0; i < gNumCubesConnected; i++){
			doit(menus[i], events[i], i);
		}
	}

	//ASSERT(e.type == MENU_EXIT);
	//m.performDefault();

	LOG("Selected Game: %d\n", e.item);

}