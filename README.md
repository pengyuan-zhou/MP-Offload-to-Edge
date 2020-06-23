Based on [ORB_SLAM2](https://github.com/raulmur/ORB_SLAM2), [Darknet](https://github.com/pjreddie/darknet), [SLAM_AR_Android](https://github.com/Martin20150405/SLAM_AR_Android), [ORB_SLAM2_with_semantic_label](https://github.com/qixuxiang/orb-slam2_with_semantic_label). 

Android client offloads Object detection and ORBSLAM2 simultaneously to same/diff server(s), 
and render both received results.

To be noticed, the code selects paths manually. Please develop your own path selection scheduler based on demand.

Publication:
```
@inproceedings{braud2020multipath,
  title={Multipath Computation Offloading for Mobile Augmented Reality},
  author={Braud, Tristan and Zhou, Pengyuan and Kangasharju, Jussi and Hui, Pan},
  booktitle={In Proceedings of the IEEE International Conference on Pervasive Computing and Communications (PerCom 2020), Austin USA},
  year={2020}
}
```

Demo:

[![Video](https://img.youtube.com/vi/V4um2N_RgZU/0.jpg)](https://www.youtube.com/watch?v=V4um2N_RgZU)
