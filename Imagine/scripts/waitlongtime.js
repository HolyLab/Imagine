var currentdate = new Date();
var targetDate = new Date("June 19, 2018 11:44:00");
print(currentdate);
print(targetDate);

while ((currentdate = new Date()) < targetDate){
     sleepms(1000);
}
print(currentdate);
