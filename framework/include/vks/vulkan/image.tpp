
#ifndef IMAGE_TPP
#define IMAGE_TPP

namespace vks {
    
    template <typename Fn>
    void Image::configure_description(Fn&& fn) {
        ImageDescription previous = m_description;
        fn(m_description);
        m_dirty = previous != m_description;
    }
    
}

#endif // IMAGE_TPP