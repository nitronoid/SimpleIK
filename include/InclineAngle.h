#ifndef INTERIORANGLE_INCLUDE_H
#define INTERIORANGLE_INCLUDE_H

#include <maya/MPxNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MEulerRotation.h>
#include <maya/MAngle.h>
#include "Utils.h"
#include <cmath>
#include <limits>
#include <functional>

template<typename TClass, const char* TTypeName>
class InclineAngleNode : public BaseNode<TClass, TTypeName> 
{
public:

  static MStatus initialize()
  {
    // Create all of our inputs
    // target location is the position our triangle arm is aimed at
    createAttribute(m_inputTargetLocation, "targetLocation", DefaultValue<MVector>());
    createAttribute(m_inputEdgeA, "staticEdgeA", 0.0);
    createAttribute(m_inputEdgeB, "staticEdgeB", 0.0);
    createAttribute(m_inputSoften, "soften", 0.0);
    createAttribute(m_inputDoSoften, "doSoften", true);

    // bend angle should be the angle between the two bones composing the triangle arm
    createAttribute(m_outputInclineAngle, "inclineAngle", DefaultValue<MAngle>(), false);

    // Tell maya about our arributes
    addAttributes(m_inputTargetLocation, m_inputEdgeA, m_inputEdgeB, m_inputSoften, m_inputDoSoften, m_outputInclineAngle);
    // Tell maya what inputs will affect our outputs (all of them)
    setAffects({m_inputTargetLocation, m_inputEdgeA, m_inputEdgeB, m_inputSoften, m_inputDoSoften}, m_outputInclineAngle);
  
    return MS::kSuccess;
  }

  virtual MStatus compute(const MPlug& _plug, MDataBlock& io_dataBlock) 
  {
    if (shouldCompute(_plug, m_outputInclineAngle)) 
    {
      AttributeData ad(io_dataBlock);
      // Get the position of our target, with no zero components
      const auto targetLocation = makeNonZero<double>(ad.get<MVector>(m_inputTargetLocation));
      // Get the two static edge lengths (the bones) 
      const auto edgeA = ad.get<double>(m_inputEdgeA);
      const auto edgeB = ad.get<double>(m_inputEdgeB);
      // Get our dynamic edge length and clamp it into our acceptable range
      const auto dynamicEdgeC = std::max(targetLocation.length(), edgeA - edgeB);
      // Calculate the softness value
      const auto dsoft = ad.get<double>(m_inputSoften) * ad.get<bool>(m_inputDoSoften);  
      // Soften our dynamic edge if required
      const auto edgeC = softenEdge(dynamicEdgeC, edgeA + edgeB, dsoft);
      // Use the law of cosines to calculate interior incline angle of the triangle
      // We add pi to get the obtuse complement angle
      const auto inclineAngle = getAngle(edgeA, edgeC, edgeB) + std::atan(clamp(targetLocation.y / targetLocation.x, -1.0, 1.0));
      // Output the values
      ad.set(m_outputInclineAngle, MAngle(inclineAngle));
  
      return MS::kSuccess;
    }
    return MS::kUnknownParameter;
  }

private:
  static Attribute m_inputTargetLocation;
  static Attribute m_inputEdgeA;
  static Attribute m_inputEdgeB;
  static Attribute m_inputSoften;
  static Attribute m_inputDoSoften;
  static Attribute m_outputInclineAngle;
};

#define MEMDECL(NAME) \
template<typename TClass, const char* TTypeName> \
Attribute InclineAngleNode<TClass, TTypeName>::NAME

MEMDECL(m_inputTargetLocation);
MEMDECL(m_inputEdgeA);
MEMDECL(m_inputEdgeB);
MEMDECL(m_inputSoften);
MEMDECL(m_inputDoSoften);
MEMDECL(m_outputInclineAngle);

#undef MEMDECL

#define INCLINEANGLE_NODE(NodeName) \
TEMPLATE_PARAMETER_LINKAGE char name##NodeName[] = #NodeName; \
class NodeName : public InclineAngleNode<NodeName, name##NodeName> {};

INCLINEANGLE_NODE(inclineAngle);

#undef INTERIORANGLE_NODE

#endif //INTERIORANGLE_INCLUDE_H
