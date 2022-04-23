#include "stdafx.h"

#include "Application.h"

int main(int argc, char *argv[]) {
for(int i = 0;i<argc;++i){
   LOG("%i: %s", i, argv[i]);
}

   Application app;
   app.Start(argc, argv);
   app.Run();
   app.Destroy();
}