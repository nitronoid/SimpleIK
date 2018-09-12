import maya.cmds as cmds

cmds.file(f=True, new=True)

def setup():
  joints = []
  vals = [0, 4, 2]
  for i in range(3):
      
      jnt = cmds.joint()
      cmds.xform(jnt, t=[vals[i],0,0])
      joints.append(jnt)
  
  loc1 = cmds.spaceLocator(n="target")[0]
  loc2 = cmds.spaceLocator(n="pole")[0]
  
  cmds.xform(loc1, t=[6,0,0])
  cmds.xform(loc2, t=[6,1,0])
  
  cmds.flushUndo()
  cmds.unloadPlugin('simpleik')
  cmds.loadPlugin('simpleik.so')
  
  tbik = cmds.createNode('sik_twoBoneIK', n='tbik1')
  
  cmds.setAttr(tbik + ".staticEdgeA", vals[1])
  cmds.setAttr(tbik + ".staticEdgeB", vals[2])
  cmds.connectAttr(loc1 + ".t", tbik + ".targetLocation")
  cmds.connectAttr(loc2 + ".t", tbik + ".poleVector")
  cmds.connectAttr(tbik + ".orientation", joints[0] + ".r")
  cmds.connectAttr(tbik + ".bendAngle", joints[1] + ".rz")
  cmds.connectAttr(tbik + ".stretchedEdgeA", joints[1] + ".tx")
  cmds.connectAttr(tbik + ".stretchedEdgeB", joints[2] + ".tx")

