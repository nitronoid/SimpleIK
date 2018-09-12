#include <maya/MFnPlugin.h>

#include "../include/TwoBoneIK.h"
#include "../include/InclineAngle.h"

MStatus initializePlugin(MObject _pluginObj)
{
  MStatus plugStat;
  MFnPlugin pluginFn(_pluginObj, "Simple two bone ik node.", "1.0.0", "Any", &plugStat);
  // only add the node if the plugin init was successful
  if (!plugStat.error()) 
  {
    int id = 0x416D;
    MStatus stat;
    #define REGISTER_MNODE(NAME)\
    stat = NAME::registerNode(pluginFn, id++);\
    CHECK_MSTATUS(stat);\
    if (!stat) plugStat = stat

    REGISTER_MNODE(twoBoneIK);
    REGISTER_MNODE(inclineAngle);

    #undef REGISTER_MNODE
  }
  return plugStat;
}

MStatus uninitializePlugin(MObject _pluginObj)
{
  MFnPlugin pluginFn(_pluginObj);
  MStatus plugStat, stat;

  #define DEREGISTER_MNODE(NAME)\
  stat = NAME::deregisterNode(pluginFn);\
  CHECK_MSTATUS(stat);\
  if (!stat) plugStat = stat

  DEREGISTER_MNODE(twoBoneIK);
  DEREGISTER_MNODE(inclineAngle);

  #undef DEREGISTER_MNODE
  return plugStat;
}

