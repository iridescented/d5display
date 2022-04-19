``//Set up Output Variables
int32 LoadSw[3]; //0-LoadSw1, 1-LoadSw2, 2-LoadSw3
int32 BatteryMode[2]; //0-Discharge, 1-Charge

//introduce an extra variable to count the total demand
int32 count; //batt percent
float32 sustain = Wind + Pv; //calculate current from Wind and PV

int32 LoadCallArr[3] = {LoadCall1,LoadCall2,LoadCall3};

//calculate demand

swtich(LoadCall1*100 + LoadCall2*10 + LoadCall3)
case 0:
	return;
case 1:
	return;
case 10:
	return;
case 100:
	return;
case 11:
	return;
case 110:
	return;
case 101:
	return;
case 111:
	LoadSw[0] = 1;
	return;
default:
	return;



if (sustain == 4) {
	MainsReq = 5;//1A, now have 5A
	LoadSw1 = 1;
	LoadSw2 = 1;
	LoadSw3 = 1;
	CBattery = 1;
	count++;
}
else if (sustain >= 3) {//have 3A
	LoadSw1 = 1;
	LoadSw2 = 1;
	LoadSw3 = 1;
	if (count > 0) {//1A, now have 4A
		count--;
	}
	else {//3A
		MainsReq = 10; //5A
		CBattery = 1;
		count++;
	}
}
else if (sustain >= 2.2) { //have 2.2A
	LoadSw1 = 1;
	LoadSw2 = 1;
	LoadSw3 = 1;	
	if (count > 0) {//1A, now have 3.2A
		DBattery = 1;
		MainsReq = 4; //0.8A, now have 4A
		count--;
	}
	else {//2.2A
		MainsReq = 9;//1.8A, now have 4A
	}
	
}
else if (sustain >= 1.2) {//1.2A
	LoadSw1 = 1;
	LoadSw2 = 1;
	if (count > 0) {//2.2A
		DBattery = 1;
		MainsReq = 9; //1.8A, now have 4A
		LoadSw3 = 1;
		count--;
	}
	else {
		MainsReq = 10;//2A, now have 3.2A
		LoadSw3 = 0;//give up load 3
	}

}
else if (sustain >= 0.2) {//0.2A
	LoadSw1 = 1;
	MainsReq = 10; //2A
	if (count > 0) {//1.2A
		DBattery = 1;
		LoadSw2 = 1;
		LoadSw3 = 0;//give up load 3
		count--;
	}
	else {
		LoadSw2 = 0;//give up load 2
		LoadSw3 = 1;
	}
}
else {//sustainable energy too low
	LoadSw1 = 1;
	if (count > 0) {//1A
		DBattery = 1;
		MainsReq = 5; //1A, now have 2A
		LoadSw2 = 0;//give up load 2
		LoadSw3 = 1;
		count--;
	}
	else {
		MainsReq = 10;//1A, now have 2A
		LoadSw2 = 0; //give up load 2
		LoadSw3 = 1;
	}
	
}