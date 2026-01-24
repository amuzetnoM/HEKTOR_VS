import React, { useRef, useEffect, useState } from 'react';
import { Canvas, useFrame, useThree } from '@react-three/fiber';
import { OrbitControls, PerspectiveCamera, Grid } from '@react-three/drei';
import * as THREE from 'three';

interface VectorPoint {
  id: string;
  position: [number, number, number];
  color: string;
  distance?: number;
}

interface VectorSpaceProps {
  vectors: VectorPoint[];
  geometry?: 'euclidean' | 'hyperbolic' | 'parabolic' | 'spherical';
  onVectorClick?: (vector: VectorPoint) => void;
}

// Vector point component
function VectorPointMesh({ point, onClick }: { point: VectorPoint; onClick?: () => void }) {
  const meshRef = useRef<THREE.Mesh>(null);
  const [hovered, setHovered] = useState(false);
  
  useFrame((state) => {
    if (meshRef.current && hovered) {
      meshRef.current.scale.setScalar(1.2 + Math.sin(state.clock.elapsedTime * 3) * 0.1);
    } else if (meshRef.current) {
      meshRef.current.scale.setScalar(1);
    }
  });
  
  return (
    <mesh
      ref={meshRef}
      position={point.position}
      onClick={onClick}
      onPointerOver={() => setHovered(true)}
      onPointerOut={() => setHovered(false)}
    >
      <sphereGeometry args={[0.05, 16, 16]} />
      <meshStandardMaterial
        color={point.color}
        emissive={point.color}
        emissiveIntensity={hovered ? 0.5 : 0.2}
        metalness={0.5}
        roughness={0.3}
      />
    </mesh>
  );
}

// Main scene component
function Scene({ vectors, geometry, onVectorClick }: VectorSpaceProps) {
  const { camera } = useThree();
  
  useEffect(() => {
    camera.position.set(5, 5, 5);
  }, [camera]);
  
  return (
    <>
      {/* Lighting */}
      <ambientLight intensity={0.5} />
      <pointLight position={[10, 10, 10]} intensity={1} />
      <pointLight position={[-10, -10, -10]} intensity={0.5} />
      
      {/* Grid */}
      <Grid
        args={[20, 20]}
        cellSize={0.5}
        cellThickness={0.5}
        cellColor="#6e6e6e"
        sectionSize={2}
        sectionThickness={1}
        sectionColor="#9d4b4b"
        fadeDistance={25}
        fadeStrength={1}
        followCamera={false}
      />
      
      {/* Coordinate axes */}
      <axesHelper args={[5]} />
      
      {/* Vector points */}
      {vectors.map((point) => (
        <VectorPointMesh
          key={point.id}
          point={point}
          onClick={() => onVectorClick?.(point)}
        />
      ))}
      
      {/* Camera controls */}
      <OrbitControls
        enableDamping
        dampingFactor={0.05}
        rotateSpeed={0.5}
        zoomSpeed={0.8}
      />
    </>
  );
}

// Main component
export default function VectorSpace3D({ vectors, geometry = 'euclidean', onVectorClick }: VectorSpaceProps) {
  return (
    <div style={{ width: '100%', height: '100%', position: 'relative' }}>
      <Canvas>
        <PerspectiveCamera makeDefault fov={75} position={[5, 5, 5]} />
        <Scene vectors={vectors} geometry={geometry} onVectorClick={onVectorClick} />
      </Canvas>
      
      {/* Info overlay */}
      <div style={{
        position: 'absolute',
        top: 16,
        left: 16,
        padding: '12px 16px',
        background: 'var(--bg-secondary)',
        border: '1px solid var(--border-color)',
        borderRadius: '8px',
        fontSize: '14px',
        color: 'var(--text-primary)'
      }}>
        <div><strong>Geometry:</strong> {geometry}</div>
        <div><strong>Vectors:</strong> {vectors.length}</div>
        <div style={{ marginTop: 8, fontSize: 12, color: 'var(--text-tertiary)' }}>
          Left-click: Rotate | Right-click: Pan | Scroll: Zoom
        </div>
      </div>
    </div>
  );
}
