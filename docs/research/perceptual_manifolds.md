# PERCEPTUAL <br> MANIFOLDS

The history of information theory and data compression has reached a critical juncture where classical signal-processing techniques, once sufficient for low-fidelity transmission, can no longer meet the demands of high-dimensional, semantically-rich media. For decades, the industry relied on the mathematical foundations laid by Shannon, focusing on the reduction of statistical redundancy within raw waveforms. However, the contemporary landscape is defined by a transition toward structured, perceptual, and latent quantization paradigms. This evolution is characterized by a move from independent scalar operations to manifold-aware vector quantization, from signal-space error minimization to perceptual-space fidelity, and from open-loop fixed curves to closed-loop, display-aware systems. Furthermore, the rise of neural latent quantization and event-based temporal modeling suggests a future where bits are allocated based on meaning rather than amplitude, and where the compression architecture accounts for the prior knowledge of the human observer.

## The Structural Evolution: From Scalar to Riemannian Quantization

The fundamental unit of lossy compression, the quantizer, has undergone a structural transformation. Early digital systems utilized scalar quantization (SQ), where each individual sample—whether a pixel intensity or an audio pulse—was mapped independently to the nearest discrete level. While SQ is computationally efficient and easily implemented in hardware, it is inherently limited because it treats multi-dimensional data as a collection of isolated points, thereby wasting the intrinsic structural correlations present in natural signals.

### The Mathematical Inefficiency of Scalar Paradigms

Scalar quantization operates on the premise of minimizing the mean square error (MSE) for a single dimension. The Lloyd-Max quantizer remains the gold standard for this approach, characterized by a set of boundary points $t_k$ and reconstruction levels $r_k$ that satisfy the centroid and nearest-neighbor conditions. For a probability density function $f_X(x)$, the MSE is expressed as:

Despite these optimizations, scalar quantization fails to exploit the "space-filling gain" that becomes available in higher dimensions. Research indicates that vector quantization (VQ) provides better rate-distortion performance even for random vectors with independent dimensions, a phenomenon attributed to the more efficient partitioning of the multi-dimensional space. VQ achieves this by mapping blocks of data to a finite set of representative vectors in a codebook, effectively capturing the density of the data distribution.

### Complexity Reduction and Lattice Quantization

The primary barrier to adopting high-dimensional VQ has historically been its exponential design complexity, often reaching $O(N!)$ or higher for unstructured codebooks. To bridge the gap between SQ and VQ, researchers have proposed unified quantization systems that utilize transformations and scalar quantizers to approximate the performance of optimal lattices. One such innovation is the tri-axis coordinate system, which converts a two-axis representation into a three-axis system, inspired by the hexagonal lattices that are mathematically optimal for two-dimensional uniform distributions. This approach reduces design complexity to $O(N^2)$ while achieving bit-rate savings of 0.4% to 24.5% across various signal distributions, including circular and elliptical Gaussian models.

| Quantization Framework | | Complexity | | Structural Awareness | | Performance Gain |
|---|---|---|---|---|---|---|
| Standard Scalar (SQ) | | O(N) | | None (Independent) | | Baseline |
| Standard Vector (VQ) | | O(N!) | | High (Global) | | Maximum (Theoretical) |
| Tri-Axis Lattice | | O(N^2) | | Moderate (Local) | | 0.4% - 24.5% over SQ |
| RSAVQ (Riemannian) | | Adaptive | | Geometric (Manifold) | | Superior at ultra-low bitrates |

### Riemannian Sensitivity and Information Geometry

In the context of modern machine learning, particularly for Large Language Models (LLMs), the parameters of a network do not reside in a flat Euclidean space but rather on a Riemannian manifold with non-uniform curvature. Traditional post-training quantization (PTQ) methods that assume Euclidean geometry often lead to unconstrained direction error and suboptimal bit allocation at extremely low bitrates, such as 2 to 4 bits.

The Riemannian Sensitivity-Aware Vector Quantization (RSAVQ) framework addresses this by leveraging information geometry to model the parameter space. By utilizing the Fisher Information Matrix (FIM), RSAVQ characterizes the local geometric structure, including inter-parameter correlations and curvature. This allows for the implementation of Error Direction Sensitivity Guidance (EDSG), which projects quantization errors onto low-sensitivity directions—specifically along the negative natural gradient directions on the manifold. This geometric awareness ensures that perturbations caused by quantization have a minimal impact on the model's loss function, preserving accuracy even when the data is compressed to its absolute limits.

## The Perceptual Shift: Quantizing Appearance and Primitives

The transition from signal space to perceptual space represents a philosophical shift in compression: the goal is no longer to reconstruct the exact pixel values, but to preserve the visual experience of the viewer. This requires a move away from generic error metrics toward models that understand contrast, texture, and structural integrity.

### Luminance Mapping and the Texture Acuity Limit

In High Dynamic Range (HDR) systems, the Perceptual Quantizer (PQ) was designed to align digital signals with the human visual system's (HVS) dynamic range, which spans roughly fourteen orders of magnitude after adaptive adjustments. However, a fixed PQ curve often fails to prevent color banding and pseudo-contouring unless a 12-bit depth is maintained. New luminance mapping relationships employ adaptive adjustment factors that modify the Just Noticeable Difference (JND) fraction based on brightness levels. This ensures uniform perception and improved reproducible contrast while maintaining a 10-bit depth, effectively optimizing the quantization interval for the HVS's instantaneous five-order dynamic range.

Parallel to luminance optimization is the understanding of spatial frequency. The human visual system displays a "texture acuity limit," where sensitivity to naturalistic structure is primarily driven by high object spatial frequencies. Experiments involving image blur and rescaling demonstrate that high-frequency bands carry significantly more task-relevant information than low-frequency luminance. Consequently, modern quantization strategies increasingly separate low-frequency structural components from high-frequency detail, treating the latter as a set of perceptual primitives rather than random noise.

### Noise and Grain as Perceptual Primitives

For decades, noise and film grain were treated as artifacts to be removed. However, in high-fidelity imaging, these elements are essential for a realistic appearance. Modern denoising and compression algorithms, such as those tested with the Synthesized Noisy Images using Calibration (SNIC) dataset, utilize physics-based heteroscedastic noise models to treat noise as a structured component. Algorithms like the Locally Noise Prior Estimation (LoNPE) and the Conditional Denoising Transformer (Condformer) integrate the noise prior into a self-attention mechanism, allowing the system to distinguish between critical image details and the imaging environment's stochastic properties.

By treating texture, noise, and grain as primitives, encoders can achieve extreme compression by transmitting the "recipe" for these textures rather than the textures themselves. This is further supported by the Perceptual Image Quality Assessment (PIQA) metric, which integrates contrast masking and neighborhood masking into a luminance, structure, and contrast comparison framework. This allows the encoder to ignore perturbations that are "masked" by existing textures, thereby saving bits for areas where the eye is more sensitive to distortion.

### Material Perception and Glossiness

Perceptual quantization also accounts for higher-level visual cues such as material properties. Research into glossiness perception indicates that the HVS is highly sensitive to the statistics of pixel-based and sub-band luminance histograms, particularly positive skewness. Specular highlights, which occupy a small total area but reside in the high luminance range, create the long tail of these histograms. Quantization schemes that preserve the spatial alignment of specular and diffuse reflectance components—and the three-dimensional shapes extracted from object contours—are essential for maintaining the perceived realism of materials like wood, textile, or polished metal.

## Closed-Loop Systems: Display-Aware and Environment-Aware Feedback

The traditional "open-loop" quantization model assumes that the encoder has no knowledge of the end-user's display characteristics or viewing environment. In contrast, closed-loop systems introduce feedback mechanisms that allow the quantization process to adapt in real-time.

### Saliency-Predicted and Eye-Tracked Bit Allocation

The most effective way to save bits is to avoid quantizing what the viewer cannot see. Saliency prediction models, often based on deep learning and Transformer architectures like MDS-ViTNet or TranSalNet, simulate the human attention mechanism. These models generate attention maps that predict the probability of a human observer fixating on specific regions of a scene. By using an encoder-decoder structure—often employing a Swin Transformer as the backbone—these systems can embed the most important features and allocate higher quantization precision to foveal regions while drastically reducing the bitrate for the periphery.

State-of-the-art saliency models are evaluated using metrics such as the Similarity Metric (SIM), Correlation Coefficient (CC), Normalized Scanpath Saliency (NSS), and Kullback-Leibler Divergence (KLD). The performance of these models is approaching human inter-observer levels, particularly when trained on diverse datasets like WIC640, which includes multimodal signals like eye-tracking, EEG, and GSR.

| Saliency Metric | | Definition | | Purpose |
|---|---|---|---|---|
| Similarity (SIM) | | Sum of minimum values at each pixel | | Measures distribution similarity |
| CC | | Correlation Coefficient | | Measures linear relationship |
| NSS | | Normalized Scanpath Saliency | | Measures saliency at fixation points |
| KLD | | Kullback-Leibler Divergence | | Measures information loss between distributions |

The integration of high-speed, closed-loop eye-trackers like EyeLoop—which operates at over 1,000 frames per second on consumer hardware—enables real-time display-aware quantization. This allows the system to adjust the quantization lattice dynamically based on the viewer's gaze, a technique that is vital for virtual reality, medical imaging, and remote robotics.

### Demographic and Cultural Sensitivity in Quantization

Research has uncovered that gaze behavior patterns vary significantly across demographics. For example, female-trained saliency models have shown different performance characteristics compared to male-trained models when evaluated on the WIC640 dataset. These differences suggest that a truly adaptive quantization system must be demographic-aware. Factors such as age and cultural background influence how individuals navigate web interfaces or natural scenes, implying that the "optimal" bit allocation for one user may be suboptimal for another. This level of personalization represents the frontier of closed-loop quantization, where the encoder-decoder-display loop incorporates the observer's profile as a primary input.

## Neural Latent Quantization: Moving Beyond Waveforms

The most significant jump in compression efficiency occurs with neural latent quantization. This paradigm involves training autoencoders to learn an optimal perceptual basis for a dataset, effectively mapping high-dimensional signals onto a lower-dimensional latent manifold.

### Autoencoders and the Latent Manifold

The core mechanism of neural latent quantization is the Variational AutoEncoder (VAE) and its discrete counterpart, the VQ-VAE. In these systems, an encoder maps the input signal x to a latent representation z, which is then quantized to a discrete codebook before being reconstructed by the decoder. This process ensures that "bits follow meaning" rather than raw amplitude. By measuring the likelihood loss in the latent space rather than the pixel space, the model avoids wasting capacity on imperceptible noise or high-entropy textures like blades of grass or sea foam.

The transition from VQ-VAE to VQGAN introduced adversarial losses and perceptual losses (like LPIPS), which allow the model to produce realistic reconstructions even when the resolution is reduced by a factor of 16x or more. This enables the generative model to "punch above its weight," using fewer bits to represent complex scenes by focusing on the underlying structure—the "things"—rather than the random "stuff" that fills the gaps.

### Overcoming Codebook Collapse and Semantic Inconsistency

Despite the success of VQ-based methods, they often suffer from "codebook collapse," where the latent space of the autoencoder becomes irregular and fragmented, leading to a small subset of the codebook being utilized. The VAEVQ framework addresses this by replacing the standard autoencoder with a VAE for quantization. This leverages a smooth, Gaussian-distributed latent space, which facilitates more effective codeword activation.

Furthermore, the Representation Coherence Strategy (RCS) and Distribution Consistency Regularization (DCR) are employed to align the continuous and discrete latent spaces. These mechanisms ensure that the semantic meaning of a latent vector is preserved through the quantization bottleneck, allowing for precise control over downstream tasks like text transfer or image generation.

### Disentangled Representation and Modularity

Quantized-Latent Autoencoders (QLAE) further improve the explicitness of these representations by forcing the model to tease apart a dataset's underlying sources of variation. By quantizing the latent space into discrete code vectors with a separate learnable scalar codebook per dimension, and applying strong regularization, the system encourages the model to assign a consistent, modular meaning to each value. This results in a latent space where individual bits or tokens correspond to identifiable features—such as the presence of a specific object or a change in lighting—rather than abstract coefficients.

## Temporal Redundancy Beyond Motion Vectors

Traditional video compression relies on motion estimation and motion compensation (MEMC) to remove inter-frame redundancy. However, motion vectors are limited by their inability to capture complex transformations and their reliance on handcrafted subtraction operations.

### Event-Based Vision and Scale-Aware Temporal Encoding

The emergence of event cameras—which provide asynchronous, low-latency visual signals based on brightness changes—offers a new perspective on temporal modeling. Frame-based paradigms are ill-suited for these streams, as they typically apply temporal modules only at high-level features. The Scale-Aware Temporal Encoding (SATE) framework addresses this by introducing recurrent modules at lower spatial scales, where events are most dense.

SATE utilizes Decoupled Deformable-enhanced Recurrent Layers (DDRL) to model the inherent motion characteristics of event streams. This "divide-and-conquer" strategy decouples feature fusion from motion estimation, allowing the system to calibrate motion patterns and filter out noise through three distinct stages:

- **Motion Information Extraction:** Learning masks and offsets for deformable convolution based on the current frame's motion.
- **Preliminary Fusion:** Integrating current features with historical spatiotemporal features.
- **Adaptive Calibration:** Applying deformable convolution to adjust the receptive field and preserve fine-grained temporal cues.

### Memory-Aware Streams and the Time-Domain Theory

Beyond mathematical motion modeling, there is a growing interest in "memory-aware" streams that account for what the viewer already "knows." This is grounded in the "time-domain brain" theory, which posits that neural assemblies produce characteristic temporally patterned spike signals for every attribute of an object or event. In this framework, sensory information is encoded through temporal correlations and circulating patterns in reverberatory circuits.

Memory operates on holographic principles, where nonlocal, distributed temporal spike patterns encode the actual attributes—the "what"—of an event. For video compression, this implies that the encoder could leverage the viewer's internal "generative model" by sending only the "delta-saliency"—the change in perception rather than the change in pixels. If the viewer can predict the next sequence of frames based on prior knowledge (e.g., a ball falling according to gravity), those frames require significantly fewer bits.

### Spike-Timing-Dependent Plasticity (STDP) in Compression

The concept of spike-timing-dependent plasticity (STDP) suggests that neural circuits preserve and manipulate sensory information through the relative timing of spikes in the millisecond range. By integrating STDP-like mechanisms into the encoder-decoder loop, compression systems can develop a "temporary network" that supports long-term working memory of the video stream. This allows the decoder to maintain a high-fidelity representation of the scene's context, reducing the need for repeated transmission of static or predictable information.

| Temporal Framework | | Redundancy Mechanism | | Complexity | | Fidelity Focus |
|---|---|---|---|---|---|---|
| Standard MEMC (H.265) | | Motion Vectors / Residuals | | Linear | | Pixel Accuracy |
| MASTC-VC | | Spatial-Temporal-Channel Context | | Non-linear | | Multi-scale Consistency |
| SATE (Event-based) | | Recurrent Deformable Layers | | Asynchronous | | Dynamic Resolution |
| Memory-Aware | | Perception Delta / STDP | | Semantic | | Conceptual Continuity |

The Multi-scale Motion-Aware and Spatial-Temporal-Channel Contextual Coding (MASTC-VC) network represents a bridge to these advanced temporal models. By utilizing multiscale motion-aware modules (MS-MAM) and contextual modules (STCCM), it achieves 23.93% BD-rate savings over the VVC standard. This is accomplished by learning the latent representation of both intra-frame pixels and inter-frame motion, ensuring that the bitstream reflects the most accurate joint model of the spatial, temporal, and channel contexts.

## Synthesis: The Future of Semantic Information Theory

The convergence of these technologies suggests that the future of quantization is inherently meaning-centric. The move from signal space to perceptual space is not merely an incremental improvement; it is a fundamental redefinition of information.

### From Waveforms to Perceptual Symbols

The "time-domain theory" offers a blueprint for this transition. By treating neural representations as "perceptual symbols" or "semantic pointers" created through non-linear interactions, we can envision a compression standard where the bitstream acts as a trigger for the decoder's internal generative capacity. In this model, the "bits" do not represent the signal itself, but rather the instructions for the decoder to reconstruct the experience using its learned codebooks and memory traces.

### The Role of Cross-Channel Perceptual Coupling

Another critical insight is the necessity of cross-channel perceptual coupling. Traditional codecs treat color channels as independent (or use a simple YCbCr transform). However, research into the HVS shows that luminance, texture, and structure are deeply coupled. Daala's PVQ is a prime example, where the number of pulses K in the shape codebook is adapted deterministically to match the gain, and the predicted signal is energy-conserved via Householder reflections. This holistic approach ensures that the quantization of one attribute (e.g., color) does not destroy the perceptual cues provided by another (e.g., luminance contrast).

### Environment-Aware and Adaptive Systems

Finally, the shift to closed-loop systems introduces the possibility of environment-aware quantization. By integrating feedback from the display device and the ambient lighting conditions, the quantizer can adjust the JND thresholds in real-time. If the viewer is in a bright environment, the encoder can afford to quantize the darker regions more aggressively, as they will be less visible. This synergy between the encoder, the display, and the environment represents the ultimate application of "bits following meaning," where the "meaning" is defined by the specific context of the observation.

## Conclusion

The journey from scalar to structured quantization reflects the broader trajectory of digital technology: a movement away from the mechanical and toward the biological. By embracing the complexity of Riemannian manifolds, the nuances of the human visual system, and the efficiency of neural latents, we are developing a new information theory that is capable of representing the world not just as a series of numbers, but as a rich, structured experience. The integration of saliency-driven feedback, memory-aware temporal modeling, and perceptual primitives ensures that our data transmission systems will continue to evolve, achieving levels of fidelity and efficiency that were once thought to be the exclusive domain of the human brain. The era of quantizing pixels is ending; the era of quantizing perception has begun.

## Works cited

1.  Approximating Vector Quantization by Transformation and Scalar Quantization, https://qmro.qmul.ac.uk/xmlui/bitstream/123456789/7570/6/VQv1.pdf
2.  Vector Quantization: Basics, PQ, RVQ & Real‑World Use Cases - TiDB, https://www.pingcap.com/article/vector-quantization-emerging-trends-and-research/
3.  Vector quantization - Wikipedia, https://en.wikipedia.org/wiki/Vector_quantization
4.  RSAVQ: Riemannian Sensitivity-Aware Vector Quantization for Large Language Models, https://arxiv.org/html/2510.01240v1
5.  An Adaptive Luminance Mapping Scheme for High Dynamic Range Content Display - MDPI, https://www.mdpi.com/2079-9292/14/6/1202
6.  Sensitivity to naturalistic texture relies primarily on high spatial frequencies - PMC - NIH, https://pmc.ncbi.nlm.nih.gov/articles/PMC9910384/
7.  Perceptual image quality assessment based on structural similarity and visual masking | Request PDF - ResearchGate, https://www.researchgate.net/publication/257344369_Perceptual_image_quality_assessment_based_on_structural_similarity_and_visual_masking
8.  Benchmarking Denoising Algorithms with Real Photographs - ResearchGate, https://www.researchgate.net/publication/320968314_Benchmarking_Denoising_Algorithms_with_Real_Photographs
9.  Relative contributions of low- and high-luminance components to material perception | JOV, https://jov.arvojournals.org/article.aspx?articleid=2718269
10. Review of Visual Saliency Prediction: Development Process from Neurobiological Basis to Deep Models - MDPI, https://www.mdpi.com/2076-3417/12/1/309
11. A gender-aware saliency prediction system for web interfaces using deep learning and eye-tracking data - PMC, https://pmc.ncbi.nlm.nih.gov/articles/PMC12491136/
12. arXiv:2405.19501v1 [cs.CV] 29 May 2024, https://arxiv.org/pdf/2405.19501?
13. EyeLoop: An Open-Source System for High-Speed, Closed-Loop Eye-Tracking - PMC, https://pmc.ncbi.nlm.nih.gov/articles/PMC8696164/
14. Improving Semantic Control in Discrete Latent Spaces with Transformer Quantized Variational Autoencoders - ACL Anthology, https://aclanthology.org/2024.findings-eacl.97/
15. Generative modelling in latent space – Sander Dieleman, https://sander.ai/2025/04/15/latents.html
16. VAEVQ: Enhancing Discrete Visual Tokenization through Variational Modeling - arXiv, https://arxiv.org/html/2511.06863v1
17. NeurIPS Poster Disentanglement via Latent Quantization, https://neurips.cc/virtual/2023/poster/71965
18. Multiscale Motion-Aware and Spatial-Temporal-Channel Contextual Coding Network for Learned Video Compression - arXiv, https://arxiv.org/pdf/2310.12733
19. NeurIPS Poster Rethinking Scale-Aware Temporal Encoding for ..., https://neurips.cc/virtual/2025/poster/115551
20. Time-domain brain: temporal mechanisms for brain ... - Frontiers, https://www.frontiersin.org/journals/computational-neuroscience/articles/10.3389/fncom.2025.1540532/full
21. Terrence Sejnowski, PhD - Salk Institute, https://www.salk.edu/scientist/terrence-sejnowski/publications/
22. Scalar-Vector Combined Quantization - Emergent Mind, https://www.emergentmind.com/topics/scalar-vector-combined-quantization-strategy
