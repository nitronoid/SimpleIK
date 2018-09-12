#ifndef TWOBONEIK_INCLUDE_H
#define TWOBONEIK_INCLUDE_H

#include "Utils.h"

template<typename TClass, const char* TTypeName>
class TwoBoneIKNode : public BaseNode<TClass, TTypeName> 
{
public:

  static MStatus initialize()
  {
    // Create all of our inputs
    // target location is the position our triangle arm is aimed at
    createAttribute(m_inputTargetLocation, "targetLocation", DefaultValue<MVector>());
    createAttribute(m_inputEdgeA, "staticEdgeA", 0.0);
    createAttribute(m_inputEdgeB, "staticEdgeB", 0.0);
    createAttribute(m_inputPoleVector, "poleVector", DefaultValue<MVector>());
    createAttribute(m_inputTwist, "twist", DefaultValue<MAngle>());
    createAttribute(m_inputSoften, "soften", 0.0);
    createAttribute(m_inputDoSoften, "doSoften", true);
    createAttribute(m_inputStretchStrength, "stretchStrength", 1.0);

    // bend angle should be the angle between the two bones composing the triangle arm
    createAttribute(m_outputBendAngle, "bendAngle", DefaultValue<MAngle>(), false);
    // This is the orientation of the entire arm, so should be on the root joint 
    createAttribute(m_outputOrientation, "orientation", DefaultValue<MEulerRotation>(), false);
    createAttribute(m_outputStretchedEdgeA, "stretchedEdgeA", 0.0, false);
    createAttribute(m_outputStretchedEdgeB, "stretchedEdgeB", 0.0, false);

    // Tell maya about our arributes
    addAttributes(
        m_inputTargetLocation, m_inputEdgeA, m_inputEdgeB, m_inputPoleVector, m_inputTwist, m_inputSoften, m_inputDoSoften, m_inputStretchStrength,
        m_outputBendAngle, m_outputOrientation, m_outputStretchedEdgeA, m_outputStretchedEdgeB
        );
    // Tell maya what inputs will affect our outputs (all of them)
    setAffects(
        {m_inputTargetLocation, m_inputEdgeA, m_inputEdgeB, m_inputPoleVector, m_inputTwist, m_inputSoften, m_inputDoSoften, m_inputStretchStrength}, 
        m_outputBendAngle, m_outputOrientation, m_outputStretchedEdgeA, m_outputStretchedEdgeB
        );
  
    return MS::kSuccess;
  }

  virtual MStatus compute(const MPlug& _plug, MDataBlock& io_dataBlock) 
  {
    if (shouldCompute(_plug, m_outputBendAngle, m_outputOrientation, m_outputStretchedEdgeA, m_outputStretchedEdgeB)) 
    {
      AttributeData ad(io_dataBlock);
      // Get the position of our target, with no zero components
      const auto targetLocation = makeNonZero<double>(ad.get<MVector>(m_inputTargetLocation));
      // Get the two static edge lengths (the bones) 
      const auto edgeA = ad.get<double>(m_inputEdgeA);
      const auto edgeB = ad.get<double>(m_inputEdgeB);
      // Get the pole vector to calculate twist
      const auto poleVector = ad.get<MVector>(m_inputPoleVector);
      // Calculate the distance from our pole vector to the target (on the xz plane) 
      const auto d = distPointToOLine<double>({poleVector.x, poleVector.z}, {targetLocation.x, targetLocation.z});
      // Calculate the world, exterior y rotation, when x is negative we do 180 - angle
      const auto worldY = M_PI * (targetLocation.x < 0) - std::atan(targetLocation.z / targetLocation.x);
      // We use the Z as a start vector, and rotate it with the arm so that it remains relative,
      // it is then used to get the normal to our z rotated triangle base
      const auto rotatedZ = MVector(0.0, 0.0, -1.0).rotateBy(MVector::Axis::kYaxis, worldY);
      // This is a cross product
      const auto N = (targetLocation ^ rotatedZ).normal();
      // Dot product the vector from our pole to the target, to get the relative height of the pole
      const auto h = makeNonZero((poleVector - targetLocation) * N);
      // Twist is essentially now a rotated version of atan(Y/X),
      // we correct using +180 for negative heights
      const auto extraTwist = ad.get<MAngle>(m_inputTwist).asRadians();
      const auto twist = M_PI * (h < 0) + std::atan(d / h) + extraTwist;
      // Get our dynamic edge length and clamp it into our acceptable range
      const auto dynamicEdgeC = std::max(targetLocation.length(), edgeA - edgeB);
      // Calculate the softness value
      const auto dsoft = ad.get<double>(m_inputSoften) * ad.get<bool>(m_inputDoSoften);  
      const auto chainLength = edgeA + edgeB;
      // Soften our dynamic edge if required
      const auto edgeC = softenEdge(dynamicEdgeC, chainLength, dsoft);
      // Use the law of cosines to calculate interior bend angle of the triangle
      // We add pi to get the obtuse complement angle
      const auto bendAngle = getAngle(edgeA, edgeB, edgeC) + M_PI;

      // Using law of cosines to get the interior angle of the triangle, this is the interior Z rotation
      MEulerRotation rot(0.0, 0.0, getAngle(edgeA, edgeC, edgeB), MEulerRotation::RotationOrder::kZXY);
      {
        // Get the rotated base edge length of the triangle
        const auto hypot = std::sqrt(sqr(targetLocation.x) + sqr(targetLocation.z));
        // This rotation is the twist, height adjustment and the Y rotation
        const MEulerRotation exterior(
            // Apply our twist as x rotation
            twist, 
            // The world Y rotation, corrected for each quadrant
            worldY,
            // This angle needs to have an incline based on the targetLocation.y of the locator
            std::atan(targetLocation.y / hypot), 
            // We need to apply our Z rotation first, as the Y rotation affects the plane on which it is applied
            MEulerRotation::RotationOrder::kXZY
            );

        // Multiply so that the interior Z rotation comes first
        rot *= exterior;
      }
      // Reorder the rotations to the standard maya convention
      rot.reorderIt(MEulerRotation::RotationOrder::kXYZ);

      const auto stretchStrength = ad.get<double>(m_inputStretchStrength);
      const auto stretchedEdgeA = stretchEdge(edgeA, dynamicEdgeC, chainLength, stretchStrength);
      const auto stretchedEdgeB = stretchEdge(edgeB, dynamicEdgeC, chainLength, stretchStrength);
  
      // Output the values
      ad.set(m_outputBendAngle, MAngle(bendAngle));
      ad.set(m_outputOrientation, rot);
      ad.set(m_outputStretchedEdgeA, stretchedEdgeA);
      ad.set(m_outputStretchedEdgeB, stretchedEdgeB);
  
      return MS::kSuccess;
    }
    return MS::kUnknownParameter;
  }

private:
  static Attribute m_inputTargetLocation;
  static Attribute m_inputEdgeA;
  static Attribute m_inputEdgeB;
  static Attribute m_inputPoleVector;
  static Attribute m_inputTwist;
  static Attribute m_inputSoften;
  static Attribute m_inputDoSoften;
  static Attribute m_inputStretchStrength;
  static Attribute m_outputBendAngle;
  static Attribute m_outputOrientation; 
  static Attribute m_outputStretchedEdgeA;
  static Attribute m_outputStretchedEdgeB;
};

#define MEMDECL(NAME) \
template<typename TClass, const char* TTypeName> \
Attribute TwoBoneIKNode<TClass, TTypeName>::NAME

MEMDECL(m_inputTargetLocation);
MEMDECL(m_inputEdgeA);
MEMDECL(m_inputEdgeB);
MEMDECL(m_inputPoleVector);
MEMDECL(m_inputTwist);
MEMDECL(m_inputSoften);
MEMDECL(m_inputDoSoften);
MEMDECL(m_inputStretchStrength);
MEMDECL(m_outputBendAngle);
MEMDECL(m_outputOrientation);
MEMDECL(m_outputStretchedEdgeA);
MEMDECL(m_outputStretchedEdgeB);

#undef MEMDECL

#define TWOBONEIK_NODE(NodeName) \
TEMPLATE_PARAMETER_LINKAGE char name##NodeName[] = #NodeName; \
class NodeName : public TwoBoneIKNode<NodeName, name##NodeName> {};

TWOBONEIK_NODE(twoBoneIK);

#undef TWOBONEIK_NODE

#endif //TWOBONEIK_INCLUDE_H
