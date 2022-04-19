// setting up output variables
int32 LoadSw1;
int32 LoadSw2;
int32 LoadSw3;
int32 DBattery;
int32 CBattery;

float32 MainsReq;


//introduce an extra variable to count the total demand
int32 count; //batt percentage
float32 sustain, left1;


sustain = Wind + PV; //calculate current from Wind and PV

//calculate demand

if (LoadCall1 && LoadCall2 && LoadCall3) { //111, demand = 4A
	if (sustain == 4) {
		MainsReq = 5;//1A, now have 5A
		LoadSw1 = 1;
		LoadSw2 = 1;
		LoadSw3 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 3) {//have 3A
		if (count > 0) {//1A, now have 4A
			DBattery = 1;
			MainReq = 0;
			LoadSw1 = 1;
			LoadSw2 = 1;
			LoadSw3 = 1;
			count--;
		}
		else {//3A
			MainsReq = 10; //5A
			LoadSw1 = 1;
			LoadSw2 = 1;
			LoadSw3 = 1;
			CBattery = 1;
			count++;
		}
	}
	else if (sustain >= 2.2) { //have 2.2A
		if (count > 0) {//1A, now have 3.2A
			DBattery = 1;
			MainsReq = 4; //0.8A, now have 4A
			LoadSw1 = 1;
			LoadSw2 = 1;
			LoadSw3 = 1;	
			count--;
		}
		else {//2.2A
			MainsReq = 9;//1.8A, now have 4A
			LoadSw1 = 1;
			LoadSw2 = 1;
			LoadSw3 = 1;
		}
		
	}
	else if (sustain >= 1.2) {//1.2A
		if (count > 0) {//2.2A
			DBattery = 1;
			MainsReq = 9; //1.8A, now have 4A
			LoadSw1 = 1;
			LoadSw2 = 1;
			LoadSw3 = 1;
			count--;
		}
		else {
			MainsReq = 10;//2A, now have 3.2A
			LoadSw1 = 1;
			LoadSw2 = 1;
			LoadSw3 = 0;//give up load 3
		}

	}
	else if (sustain >= 0.2) {//0.2A
		if (count > 0) {//1.2A
			DBattery = 1;
			MainsReq = 10; //2A, now have 3.2A
			LoadSw1 = 1;
			LoadSw2 = 1;
			LoadSw3 = 0;//give up load 3
			count--;
		}
		else {
			MainsReq = 10; //2A
			LoadSw1 = 1;
			LoadSw2 = 0;//give up load 2
			LoadSw3 = 1;
		}
	}
	else {//sustainable energy too low
		if (count > 0) {//1A
			DBattery = 1;
			MainsReq = 5; //1A, now have 2A
			LoadSw1 = 1;
			LoadSw2 = 0;//give up load 2
			LoadSw3 = 1;
			count--;
		}
		else {
			MainsReq = 10;//1A, now have 2A
			LoadSw1 = 1;
			LoadSw2 = 0; //give up load 2
			LoadSw3 = 1;
		}
		
	}
}


else if (LoadCall1 && LoadCall2 && !LoadCall3) {//110, demand = 3.2A
	LoadSw3 = 0;//load 3 not calling
	if (sustain >= 4) {//4A
		MainsReq = 1;//0.2A, now have 4.2
		LoadSw1 = 1;
		LoadSw2 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 3.2) {//3.2A
		MainsReq = 5;//1A, now have 4.2
		LoadSw1 = 1;
		LoadSw2 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 2.2) {//2.2A
		if (count > 0) {//1A, now have 3.2A
			DBattery = 1;
			LoadSw1 = 1;
			LoadSw2 = 1;
			count--;
		}
		else {
			MainsReq = 10;//2A, now have 4.2
			LoadSw1 = 1;
			LoadSw2 = 1;
			CBattery = 1;
			count++;
		}
	}
	else if (sustain >= 1) {//1A
		if (count > 0) {//1A
			DBattery = 1;
			MainsReq = 6;//1.2A, now have 3.2A
			LoadSw1 = 1;
			LoadSw2 = 1;
			count--;
		}
		else{
			MainsReq = 6;//1.2A, now have 2.2A
			LoadSw1 = 1;
			LoadSw2 = 0;//give up load 2
			CBattery = 1;
			count++;
		}
		
	}

	else if (sustain >= 0.2) {//0.2A
		if (count > 0) {//1A
			DBattery = 1;
			MainsReq = 10;//2A, now have 3.2A
			LoadSw1 = 1;
			LoadSw2 = 1;
			count--;
		}
		else {
			MainsReq = 10;//2A, now have 2.2A
			LoadSw1 = 1;
			LoadSw2 = 0;//give up load 2
			CBattery = 1;
			count++;
		}
		
	}
	else {//sustainable energy too low
		if (count > 0) {//1A
			DBattery = 1;
			MainsReq = 1;//0.2A, now have 1.2A
			LoadSw1 = 1;
			LoadSw2 = 0; //give up load 2 
			count--;
		}
		else {
			MainsReq = 6;//1.2A
			LoadSw1 = 1;
			LoadSw2 = 0; //give up load 2 
		}
	}

}
//        1.2A          2A            0.8A
else if (LoadCall1 && !LoadCall2 && LoadCall3) { //101, demand = 2A
	LoadSw2 = 0;//load 2 not calling
	if (sustain >= 3) {//3A
		LoadSw1 = 1;
		LoadSw3 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 2) {//2A
		MainsReq = 5;//1A, now have 3A
		LoadSw1 = 1;
		LoadSw3 = 1;
		CBattery = 1;
		count++;
	}

	else if (sustain >= 1) {//1A
		if (count > 0) {//1A, now have 2A
			DBattery = 1;
			LoadSw1 = 1;
			LoadSw3 = 1;
			count--;
		}
		else {
			MainsReq = 10;//2A, now have 3A
			LoadSw1 = 1;
			LoadSw3 = 1;
			CBattery = 1;
			count++;
		}
	}
	else if (sustain >= 0.2) {//0.2A
		if (count > 0) {//1A
			MainsReq = 4;//0.8A, now have 2A
			LoadSw1 = 1;
			LoadSw3 = 1;
			count--;
		}
		else {
			MainsReq = 9;//1.8A, now have 2A
			LoadSw1 = 1;
			LoadSw3 = 1;
		}
	}
	else { //sustainable energy too low
		if (count > 0) {//1A
			MainsReq = 5;//1A, now have 2A
			LoadSw1 = 1;
			LoadSw3 = 1;
			count--;
		}
		else {
			MainsReq = 10;//2A
			LoadSw1 = 1;
			LoadSw3 = 1;
		}	
	}
}

else if (!LoadCall1 && LoadCall2 && !LoadCall3) {//010, demand = 2A
	LoadSw1 = 0;//load 1 not calling
	LoadSw3 = 0;//load 3 not calling
	if (sustain >= 3) {
		LoadSw2 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 2) {// 2A
		MainsReq = 5;//1A, now have 3A
		LoadSw2 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 1.8) {// 2A
		MainsReq = 6;//1.2A, now have 3A
		LoadSw2 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 1.6) {// 2A
		MainsReq = 7;//1.4A, now have 3A
		LoadSw2 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 1.4) {// 2A
		MainsReq = 8;//1.6A, now have 3A
		LoadSw2 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 1.2) {// 2A
		MainsReq = 9;//1.8A, now have 3A
		LoadSw2 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 1) {// 1A
		if (count > 0) {//1A, now have 2A
			LoadSw2 = 1;
			count--;
		}
		else {
			MainsReq = 10;//2A, now have 3A
			LoadSw2 = 1;
			CBattery = 1;
			count++;
		}
	}
	else {// sustainable energy too low
		if (count > 0) {//1A
			MainsReq = 5;//now have 2A
			LoadSw2 = 1;
			count--;
		}

		else {
			MainsReq = 10;//2A
			LoadSw2 = 1;
		}
	}
}

else if (!LoadCall1 && LoadCall2 && LoadCall3) { // 011,demand = 2.8
	LoadSw1 = 0;//load 1 not calling
	if (sustain >= 3.8) {
		LoadSw2 = 1;
		LoadSw3 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 2.8) {
		MainsReq = 5;//1A, now have 3.8
		LoadSw2 = 1;
		LoadSw3 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 1.8) {
		if (count > 0) {//1A, now have 2.8A
			LoadSw2 = 1;
			LoadSw3 = 1;
			count--;
		}
		else {
			MainsReq = 10;//2A, now have 3.8A
			LoadSw2 = 1;
			LoadSw3 = 1;
			CBattery = 1;
			count++;
		}
	}
	else if (sustain >= 0.8) {
		if (count > 0) {//1A
			MainsReq = 5;//1A, now have 2.8A
			LoadSw2 = 1;
			LoadSw3 = 1;
			count--;
		}
		else {
			MainsReq = 10;//2A, now have 2.8A
			LoadSw2 = 1;
			LoadSw3 = 1;
		}

	}
	else if (sustain >= 0.2) {
		if (count > 0) {//1A
			MainsReq = 8;//1.6A, now have 2.8A
			LoadSw2 = 1;
			LoadSw3 = 1;
			count--;
		}
		else {
			MainsReq = 9;//1.8A, now have 2A
			LoadSw2 = 1;
			LoadSw3 = 0;//give up load 3
		}
	}
	else {//sustainable energy too low
		if (count > 0) {//1A
			MainsReq = 9;//1.8A, now have 2.7A
			LoadSw2 = 1;
			LoadSw3 = 1;
			count--;
		}
		else {
			MainsReq = 10;//2A
			LoadSw2 = 1;
			LoadSw3 = 0;//give up load 3
		}
	}
}
else if (LoadCall1 && !LoadCall2 && !LoadCall3) {//100, demand = 1.2A
	LoadSw2 = 0;//load 2 not calling
	LoadSw3 = 0;//load 3 not calling
	if (sustain >= 2.2) {
		LoadSw1 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 1.2) {
		MainsReq = 5;//1A, now have 2.2A
		LoadSw1 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 1) {
		MainsReq = 6;//1.2A, now have 2.2A
		LoadSw1 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 0.8) {
		MainsReq = 7;//1.4A, now have 2.2A
		LoadSw1 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 0.6) {
		MainsReq = 8;//1.6A, now have 2.2A
		LoadSw1 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 0.4) {
		MainsReq = 9;//1.8A, now have 2.2A
		LoadSw1 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 0.2) {
		MainsReq = 10;//2A, now have 2.2A
		LoadSw1 = 1;
		CBattery = 1;A
		count++;
	}
	else {//sustainable energy too low
		if (count > 0) {//1A
			MainsReq = 1;//0.2A, now have 1.2A
			LoadSw = 1;
		}
		else {
			MainsReq = 6;//1.2A
			LoadSw1 = 1;
		}
	}
}
else if (!LoadCall1 && !LoadCall2 && LoadCall3) { //001, demand = 0.8
	LoadSw1 = 0;//load 1 not calling
	LoadSw2 = 0;//load 2 not calling
	if (sustain >= 1.8) {
		LoadSw3 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 1.6) {
		MainsReq = 1;//0.2A, now have 1.8A
		LoadSw3 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 1.4) {
		MainsReq = 2;//0.4A, now have 1.8A
		LoadSw3 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 1.2) {
		MainsReq = 3;//0.6A, now have 1.8A
		LoadSw3 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 1) {
		MainsReq = 4;//0.8A, now have 1.8A
		LoadSw3 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 0.8) {
		MainsReq = 5;//1A, now have 1.8A
		LoadSw3 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 0.6) {
		MainsReq = 6;//1.2A, now have 1.8A
		LoadSw3 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 0.4) {
		MainsReq = 7;//1.4A, now have 1.8A
		LoadSw3 = 1;
		CBattery = 1;
		count++;
	}
	else if (sustain >= 0.2) {
		MainsReq = 8;//1.6A, now have 1.8A
		LoadSw3 = 1;
		CBattery = 1;
		count++;
	}
	else {//sustainable energy too low
		MainsReq = 9;//1.8
		LoadSw3 = 1;
		CBattery = 1;
		count++;
	}
}
else { //no demand, 000
	LoadSw1 = 0;//load 1 not calling
	LoadSw2 = 0;//load 2 not calling
	LoadSw3 = 0;//load 3 not calling
	DBattery = 0;//no need discharge
	if (sustain >= 1) {
		MainsReq = 0;
		CBattery = 1;//1A
		count++;
	}
	else if (sustain >= 0.8) {
		DBattery = 0;
		MainsReq = 1;//0.2A, now have 1A
		CBattery = 1;//1A
		count++;
	}
	else if (sustain >= 0.6) {
		DBattery = 0;
		MainsReq = 2;//0.4A, now have 1A
		CBattery = 1;//1A
		count++;
	}
	else if (sustain >= 0.4) {
		DBattery = 0;
		MainsReq = 3;//0.6A, now have 1A
		CBattery = 1;//1A
		count++;
	}
	else if (sustain >= 0.2) {
		DBattery = 0;
		MainsReq = 4;//0.8A, now have 1A
		CBattery = 1;//1A
		count++;
	}
	else {
		LoadSw1 = 0;
		LoadSw2 = 0;
		LoadSw3 = 0;
		DBattery = 0;
		MainsReq = 5;//1A
		CBattery = 1;//1A
		count++;
	}
}



