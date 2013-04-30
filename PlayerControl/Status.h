struct Status{  			//Status hold timed status conditions
	byte Carrier;
        byte Value;
	byte num_updates;
};

Status current[3];

void pushStat(Status stat){
  for(char i=0;i<3;i++){
    if(current[i].num_updates==0xFF){
      current[i] = stat;
      break;
    }
  }
}
