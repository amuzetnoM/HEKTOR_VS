# PERCEPTUAL <br> MANIFOLDS
> Author: Ali A. Shakil  
> Date: June 12, 2024

I believe information theory and compression sit at a turning point: classical signal-processing methods that worked for low‑fidelity transmission no longer suffice for high-dimensional, semantically rich media. Where Shannon focused on reducing statistical redundancy in waveforms, we now shift toward structured, perceptual, and latent quantization: from independent scalar operations to manifold-aware vector quantization, from signal-space error minimization to perceptual fidelity, and from open-loop fixed curves to closed-loop, display-aware systems. Neural latent quantization and event-based temporal models suggest a future in which bits follow meaning rather than amplitude, and compression uses priors about the human observer.

## The Structural Evolution: From Scalar to Riemannian Quantization

The quantizer—the unit of lossy compression—has changed structurally. Scalar quantization (SQ) mapped each sample independently to discrete levels. SQ is cheap and hardware-friendly but treats multi-dimensional data as isolated points and wastes intrinsic correlations.

### The Mathematical Inefficiency of Scalar Paradigms

Scalar quantization minimizes per-dimension MSE; the Lloyd–Max quantizer is canonical, defined by boundary points t_k and reconstruction levels r_k that satisfy centroid and nearest-neighbor conditions. For a probability density function f_X(x), the MSE is expressed as:

Despite these optimizations, SQ fails to exploit the "space‑filling gain" available in higher dimensions. Vector quantization (VQ) maps blocks to codebooks and achieves better rate–distortion by partitioning the multi-dimensional space.

### Complexity Reduction and Lattice Quantization

High-dimensional VQ historically suffers exponential design complexity. To bridge SQ and VQ, unified systems use transforms and scalar quantizers to approximate lattices. For example, a tri-axis coordinate system converts a two-axis to a three-axis representation, inspired by hexagonal lattices optimal in 2D, reducing complexity to O(N^2) and yielding 0.4%–24.5% bit-rate savings across distributions like circular and elliptical Gaussians.

| Quantization Framework | | Complexity | | Structural Awareness | | Performance Gain |
|---|---|---|---|---|---|---|
| Standard Scalar (SQ) | | O(N) | | None (Independent) | | Baseline |
| Standard Vector (VQ) | | O(N!) | | High (Global) | | Maximum (Theoretical) |
| Tri-Axis Lattice | | O(N^2) | | Moderate (Local) | | 0.4% - 24.5% over SQ |
| RSAVQ (Riemannian) | | Adaptive | | Geometric (Manifold) | | Superior at ultra-low bitrates |

### Riemannian Sensitivity and Information Geometry

In modern ML—especially with LLMs—parameters live on Riemannian manifolds with nonuniform curvature. Euclidean-assuming PTQ methods can misallocate bits at ultra-low rates (2–4 bits). Riemannian Sensitivity-Aware Vector Quantization (RSAVQ) uses information geometry—via the Fisher Information Matrix (FIM)—to capture local curvature and correlations. Error Direction Sensitivity Guidance (EDSG) projects quantization errors onto low-sensitivity directions (negative natural gradients on the manifold), minimizing loss impact and preserving accuracy under extreme compression.

## The Perceptual Shift: Quantizing Appearance and Primitives

We must move from reconstructing pixel values to preserving the viewer's experience, favoring perceptual metrics over generic error metrics.

### Luminance Mapping and the Texture Acuity Limit

HDR’s Perceptual Quantizer (PQ) maps digital signals to the HVS dynamic range, but fixed PQ can cause banding unless we use 12-bit. Adaptive luminance mapping adjusts the JND fraction by brightness to maintain uniform perception at 10-bit depth. Spatial frequency matters too: the HVS relies heavily on high spatial frequencies (texture acuity limit). Experiments show high-frequency bands carry more task-relevant information, so modern methods separate low-frequency structure from high-frequency details and treat the latter as perceptual primitives.

### Noise and Grain as Perceptual Primitives

Historically treated as artifacts, noise and grain are now seen as essential for realism. Denoising and compression that use physics-based heteroscedastic noise models (e.g., tested with SNIC) treat noise as structured. Approaches like LoNPE and Condformer integrate noise priors into attention, distinguishing important details from imaging stochasticity. By sending "recipes" for textures rather than textures themselves, encoders can compress aggressively. PIQA integrates contrast and neighborhood masking into luminance, structure, and contrast comparisons, allowing encoders to ignore masked perturbations and save bits where the eye is less sensitive.

### Material Perception and Glossiness

Material cues (e.g., glossiness) depend on luminance histogram statistics such as positive skewness: specular highlights create long tails. Schemes that preserve spatial alignment of specular and diffuse components and 3D contours maintain perceived realism in materials like wood or metal.

## Closed-Loop Systems: Display-Aware and Environment-Aware Feedback

Open-loop codecs ignore end-user displays and environments. Closed-loop systems add feedback so quantization adapts in real time.

### Saliency-Predicted and Eye-Tracked Bit Allocation

We save most bits by avoiding what viewers don’t see. Saliency models (MDS‑ViTNet, TranSalNet) predict fixation probabilities and produce attention maps that let encoders allocate precision to foveal regions and lower bitrate for periphery. Common metrics are SIM, CC, NSS, and KLD, and models trained on datasets like WIC640 reach near human inter-observer levels.

| Saliency Metric | | Definition | | Purpose |
|---|---|---|---|---|
| Similarity (SIM) | | Sum of minimum values at each pixel | | Measures distribution similarity |
| CC | | Correlation Coefficient | | Measures linear relationship |
| NSS | | Normalized Scanpath Saliency | | Measures saliency at fixation points |
| KLD | | Kullback-Leibler Divergence | | Measures information loss between distributions |

High-speed eye-trackers like EyeLoop (>1,000 FPS on consumer hardware) enable real-time, display-aware quantization, dynamically adjusting lattices based on gaze—crucial for VR, medical imaging, and remote robotics.

### Demographic and Cultural Sensitivity in Quantization

Gaze behavior varies by demographics; for example, female-trained saliency models behave differently from male-trained ones on WIC640. Age and cultural background affect attention patterns, so adaptive, demographic-aware quantization is needed—personalizing bit allocation based on observer profile.

## Neural Latent Quantization: Moving Beyond Waveforms

Neural latent quantization gives the biggest compression gains by training autoencoders to learn perceptual bases—mapping signals onto lower-dimensional latent manifolds.

### Autoencoders and the Latent Manifold

VAEs and VQ‑VAEs map inputs x to latents z which are quantized to codebooks before decoding. This makes bits follow meaning, not amplitude. Measuring likelihood loss in latent space avoids wasting capacity on imperceptible noise. VQGANs added adversarial and perceptual (LPIPS) losses to reconstruct realistic images even with large resolution reductions, focusing on structure over random detail.

### Overcoming Codebook Collapse and Semantic Inconsistency

VQ methods can suffer codebook collapse (few codewords used). VAEVQ uses a VAE-style continuous latent to encourage smooth codeword activation. Representation Coherence Strategy (RCS) and Distribution Consistency Regularization (DCR) align continuous and discrete latents to preserve semantic meaning through quantization, aiding tasks like text transfer or image generation.

### Disentangled Representation and Modularity

QLAEs force disentanglement by assigning modular meanings to quantized latent dimensions, using separate learnable codebooks and strong regularization. This yields latents where bits/tokens correspond to identifiable features (object presence, lighting), not just abstract coefficients.

## Temporal Redundancy Beyond Motion Vectors

Video compression has relied on motion estimation/compensation (MEMC), but motion vectors struggle with complex transformations.

### Event-Based Vision and Scale-Aware Temporal Encoding

Event cameras produce asynchronous brightness-change streams. Frame-based modules are ill-suited; SATE brings recurrent modules to lower spatial scales with Decoupled Deformable-enhanced Recurrent Layers (DDRL). SATE’s divide-and-conquer strategy separates motion extraction, preliminary fusion, and adaptive calibration via deformable convolutions to preserve fine temporal cues.

- Motion Information Extraction: learn masks and offsets for deformable conv based on motion.
- Preliminary Fusion: integrate current features with historical spatiotemporal features.
- Adaptive Calibration: apply deformable conv to adjust receptive fields and preserve detail.

### Memory-Aware Streams and the Time-Domain Theory

The "time-domain brain" theory suggests perception encodes attributes via temporal spike patterns. Memory operates holographically, encoding attributes as distributed patterns. For video, encoders could send "delta-saliency"—changes in perception—rather than pixel deltas. Predictable sequences (e.g., gravity-driven motion) require far fewer bits.

### Spike-Timing-Dependent Plasticity (STDP) in Compression

STDP implies sensory information is stored via spike timing. Integrating STDP-like mechanisms into encoder-decoder loops can form temporary networks supporting working memory of streams, enabling decoders to maintain high-fidelity context without repeated transmission.

| Temporal Framework | | Redundancy Mechanism | | Complexity | | Fidelity Focus |
|---|---|---|---|---|---|---|
| Standard MEMC (H.265) | | Motion Vectors / Residuals | | Linear | | Pixel Accuracy |
| MASTC-VC | | Spatial-Temporal-Channel Context | | Non-linear | | Multi-scale Consistency |
| SATE (Event-based) | | Recurrent Deformable Layers | | Asynchronous | | Dynamic Resolution |
| Memory-Aware | | Perception Delta / STDP | | Semantic | | Conceptual Continuity |

MASTC-VC uses MS-MAM and STCCM to jointly model intra-frame pixels and inter-frame motion, yielding 23.93% BD-rate savings over VVC by learning a joint spatial–temporal–channel latent.

## Synthesis: The Future of Semantic Information Theory

These threads converge on meaning-centric quantization. Moving from waveforms to perceptual symbols means the bitstream becomes instructions for the decoder’s generative capacity rather than raw signal carriers.

### From Waveforms to Perceptual Symbols

Treating neural representations as perceptual symbols or semantic pointers suggests a standard where bits trigger the decoder to reconstruct experience using learned codebooks and memory traces; bits represent reconstruction instructions, not the signal itself.

### The Role of Cross-Channel Perceptual Coupling

Cross-channel coupling matters: luminance, texture, and structure are intertwined. Daala’s PVQ adapts pulses K deterministically to gain and conserves energy via Householder reflections—this holistic approach preserves perceptual cues across attributes.

### Environment-Aware and Adaptive Systems

Closed-loop, environment-aware quantization integrates display and ambient lighting feedback to adjust JND thresholds in real time. In bright conditions, darker regions can be quantized more aggressively. This encoder–display–environment synergy is the practical embodiment of "bits following meaning."

## Conclusion

The shift from scalar to structured quantization mirrors a move from mechanical to biological principles. Riemannian manifolds, HVS nuances, and neural latents point to an information theory capable of representing the world as structured experience. Saliency-driven feedback, memory-aware temporal models, and perceptual primitives mean our transmission systems will reach fidelity and efficiency once thought unique to the human brain. The era of quantizing pixels is ending; we are entering the era of quantizing perception.

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
