# August 1, 2023

## Post Mortem

### Impressions

I had a lot of fun this month, and I'm really glad I took the time to do more graphics programming. I forgot how much fun I have with it, and I learned some really cool stuff. I'm a little sad it ended on a bit of a sour note since I couldn't finish my work, but that's what happens when I get a little carried away and forget about scope or time! I'll still spend some time fixing the last of it, but beyond that I'm generally happy with what I've done this month.

### What I made

In this month I implemented chromatic aberration as a post-process effect, and learned how to use compute shaders. I then converted some of my particle emitters to using compute shaders, and toyed around with doing even more on the GPU. While that part didn't work out, I was able to get quite close with binding multiple resources that were shared and updated between multiple different shader passes. 

### What I learned

The big thing I learned was binding resources and using them for compute shaders. It's really cool having this new aspect of GPU programming open-up to me where I can update GPU resources and just have them passed between each other without using the CPU for those calculations. Beyond that I learned some more stuff about how the GPU actually behaves under the hood, using caching and other techniques to make my shader code run efficiently.

### Conclusion

Like last month I didn't get a lot done in the grand scheme of things, but I'm very happy with my progress! I'm learning cool things and enjoying myself during it, so what's not to like? I don't have much else to say beyond that, but I need to start thinking about what I'm going to do for August (maybe some audio stuffs).